/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony.gsm;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.DataConnectionTracker;
import com.android.internal.telephony.EventLogTags;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.MccTable;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.RestrictedState;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.ServiceStateTracker;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.TelephonyProperties;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.provider.Telephony.Intents;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.gsm.GsmCellLocation;
import android.text.TextUtils;
import android.util.EventLog;
import android.util.Log;
import android.util.TimeUtils;

import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 * {@hide}
 */
final class GsmServiceStateTracker extends ServiceStateTracker {
    static final String LOG_TAG = "GSM";
    static final boolean DBG = true;

    GSMPhone phone;
    GsmCellLocation cellLoc;
    GsmCellLocation newCellLoc;
    int mPreferredNetworkType;

    private int gprsState = ServiceState.STATE_OUT_OF_SERVICE;
    private int newGPRSState = ServiceState.STATE_OUT_OF_SERVICE;
    private int mMaxDataCalls = 1;
    private int mNewMaxDataCalls = 1;
    private int mReasonDataDenied = -1;
    private int mNewReasonDataDenied = -1;

    /**
     * GSM roaming status solely based on TS 27.007 7.2 CREG. Only used by
     * handlePollStateResult to store CREG roaming result.
     */
    private boolean mGsmRoaming = false;

    /**
     * Data roaming status solely based on TS 27.007 10.1.19 CGREG. Only used by
     * handlePollStateResult to store CGREG roaming result.
     */
    private boolean mDataRoaming = false;

    /**
     * Mark when service state is in emergency call only mode
     */
    private boolean mEmergencyOnly = false;

    /**
     * Sometimes we get the NITZ time before we know what country we
     * are in. Keep the time zone information from the NITZ string so
     * we can fix the time zone once know the country.
     */
    private boolean mNeedFixZone = false;
    private int mZoneOffset;
    private boolean mZoneDst;
    private long mZoneTime;
    private boolean mGotCountryCode = false;
    private ContentResolver cr;

    String mSavedTimeZone;
    long mSavedTime;
    long mSavedAtTime;

    /**
     * We can't register for SIM_RECORDS_LOADED immediately because the
     * SIMRecords object may not be instantiated yet.
     */
    private boolean mNeedToRegForSimLoaded;

    /** Started the recheck process after finding gprs should registered but not. */
    private boolean mStartedGprsRegCheck = false;

    /** Already sent the event-log for no gprs register. */
    private boolean mReportedGprsNoReg = false;

    /**
     * The Notification object given to the NotificationManager.
     */
    private Notification mNotification;

    /** Wake lock used while setting time of day. */
    private PowerManager.WakeLock mWakeLock;
    private static final String WAKELOCK_TAG = "ServiceStateTracker";

    /** Keep track of SPN display rules, so we only broadcast intent if something changes. */
    private String curSpn = null;
    private String curPlmn = null;
    private int curSpnRule = 0;

    /** waiting period before recheck gprs and voice registration. */
    static final int DEFAULT_GPRS_CHECK_PERIOD_MILLIS = 60 * 1000;

    /** Notification type. */
    static final int PS_ENABLED = 1001;            // Access Control blocks data service
    static final int PS_DISABLED = 1002;           // Access Control enables data service
    static final int CS_ENABLED = 1003;            // Access Control blocks all voice/sms service
    static final int CS_DISABLED = 1004;           // Access Control enables all voice/sms service
    static final int CS_NORMAL_ENABLED = 1005;     // Access Control blocks normal voice/sms service
    static final int CS_EMERGENCY_ENABLED = 1006;  // Access Control blocks emergency call service

    /** Notification id. */
    static final int PS_NOTIFICATION = 888;  // Id to update and cancel PS restricted
    static final int CS_NOTIFICATION = 999;  // Id to update and cancel CS restricted

    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_LOCALE_CHANGED)) {
                // update emergency string whenever locale changed
                // +[bug0001494 android_team jackyyang 20120314] fix senario of "no plmn"
                String operatorId = phone.mIccRecords.getOperatorNumeric();
                String operatorName = findOperatorNameForChina(operatorId);
                if(ss.getOperatorAlphaLong()!=null && operatorName!=null) {
                    ss.setOperatorAlphaLong(operatorName);
                    phone.setSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_ALPHA,
                            ss.getOperatorAlphaLong());
                    phone.notifyServiceStateChanged(ss);
                }
                // -[bug0001494 android_team jackyyang 20120314]

                updateSpnDisplay();
            }
        }
    };

    private ContentObserver mAutoTimeObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            Log.i("GsmServiceStateTracker", "Auto time state changed");
            revertToNitzTime();
        }
    };

    private ContentObserver mAutoTimeZoneObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            Log.i("GsmServiceStateTracker", "Auto time zone state changed");
            revertToNitzTimeZone();
        }
    };

    public GsmServiceStateTracker(GSMPhone phone) {
        super();

        this.phone = phone;
        cm = phone.mCM;
        ss = new ServiceState();
        newSS = new ServiceState();
        cellLoc = new GsmCellLocation();
        newCellLoc = new GsmCellLocation();
        mSignalStrength = new SignalStrength();

        PowerManager powerManager =
                (PowerManager)phone.getContext().getSystemService(Context.POWER_SERVICE);
        mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, WAKELOCK_TAG);

        cm.registerForAvailable(this, EVENT_RADIO_AVAILABLE, null);
        cm.registerForRadioStateChanged(this, EVENT_RADIO_STATE_CHANGED, null);

        cm.registerForVoiceNetworkStateChanged(this, EVENT_NETWORK_STATE_CHANGED, null);
        cm.setOnNITZTime(this, EVENT_NITZ_TIME, null);
        cm.setOnSignalStrengthUpdate(this, EVENT_SIGNAL_STRENGTH_UPDATE, null);
        cm.setOnRestrictedStateChanged(this, EVENT_RESTRICTED_STATE_CHANGED, null);
        cm.registerForSIMReady(this, EVENT_SIM_READY, null);

        // system setting property AIRPLANE_MODE_ON is set in Settings.
        int airplaneMode = Settings.System.getInt(
                phone.getContext().getContentResolver(),
                Settings.System.AIRPLANE_MODE_ON, 0);
        mDesiredPowerState = ! (airplaneMode > 0);
        //add by alex for control modem power
        phone.setSystemProperty(TelephonyProperties.PROPERTY_AIRPLANE_MODE,
                airplaneMode > 0 ? "on" : "off");

        cr = phone.getContext().getContentResolver();
        cr.registerContentObserver(
                Settings.System.getUriFor(Settings.System.AUTO_TIME), true,
                mAutoTimeObserver);
        cr.registerContentObserver(
                Settings.System.getUriFor(Settings.System.AUTO_TIME_ZONE), true,
                mAutoTimeZoneObserver);

        setSignalStrengthDefaultValues();
        mNeedToRegForSimLoaded = true;

        // Monitor locale change
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_LOCALE_CHANGED);
        phone.getContext().registerReceiver(mIntentReceiver, filter);

        // Gsm doesn't support OTASP so its not needed
        phone.notifyOtaspChanged(OTASP_NOT_NEEDED);
    }

    public void dispose() {
        // Unregister for all events.
        cm.unregisterForAvailable(this);
        cm.unregisterForRadioStateChanged(this);
        cm.unregisterForVoiceNetworkStateChanged(this);
        cm.unregisterForSIMReady(this);

        phone.mIccRecords.unregisterForRecordsLoaded(this);
        cm.unSetOnSignalStrengthUpdate(this);
        cm.unSetOnRestrictedStateChanged(this);
        cm.unSetOnNITZTime(this);
        cr.unregisterContentObserver(this.mAutoTimeObserver);
        cr.unregisterContentObserver(this.mAutoTimeZoneObserver);
    }

    protected void finalize() {
        if(DBG) log("finalize");
    }

    @Override
    protected Phone getPhone() {
        return phone;
    }

    public void handleMessage (Message msg) {
        AsyncResult ar;
        int[] ints;
        String[] strings;
        Message message;

        switch (msg.what) {
            case EVENT_RADIO_AVAILABLE:
                //this is unnecessary
                //setPowerStateToDesired();
                break;

            case EVENT_SIM_READY:
                // Set the network type, in case the radio does not restore it.
                cm.setCurrentPreferredNetworkType();

                // The SIM is now ready i.e if it was locked
                // it has been unlocked. At this stage, the radio is already
                // powered on.
                if (mNeedToRegForSimLoaded) {
                    phone.mIccRecords.registerForRecordsLoaded(this,
                            EVENT_SIM_RECORDS_LOADED, null);
                    mNeedToRegForSimLoaded = false;
                }

                boolean skipRestoringSelection = phone.getContext().getResources().getBoolean(
                        com.android.internal.R.bool.skip_restoring_network_selection);

                if (!skipRestoringSelection) {
                    // restore the previous network selection.
                    phone.restoreSavedNetworkSelection(null);
                    /** [Bug0001532 Nusmart alex. for quicker register.] **/
                    pollNetworkState_f = phone.getContext().getResources().getBoolean(
                          com.android.internal.R.bool.config_radioQuickRegister);
                    /** [Bug0001532 Nusmart alex. for quicker register.] **/
                }
                pollState();

                // Signal strength polling stops when radio is off
                queueNextSignalStrengthPoll();
                break;

            case EVENT_RADIO_STATE_CHANGED:
                // This will do nothing in the radio not
                // available case
                setPowerStateToDesired();
                pollState();
                break;

            case EVENT_NETWORK_STATE_CHANGED:
                pollState();
                break;

            case EVENT_GET_SIGNAL_STRENGTH:
                // This callback is called when signal strength is polled
                // all by itself

                if (!(cm.getRadioState().isOn()) || (cm.getRadioState().isCdma())) {
                    // Polling will continue when radio turns back on and not CDMA
                    return;
                }
                ar = (AsyncResult) msg.obj;
                onSignalStrengthResult(ar);
                queueNextSignalStrengthPoll();

                break;

            case EVENT_GET_LOC_DONE:
                ar = (AsyncResult) msg.obj;

                if (ar.exception == null) {
                    String states[] = (String[])ar.result;
                    int lac = -1;
                    int cid = -1;
                    if (states.length >= 3) {
                        try {
                            if (states[1] != null && states[1].length() > 0) {
                                lac = Integer.parseInt(states[1], 16);
                            }
                            if (states[2] != null && states[2].length() > 0) {
                                cid = Integer.parseInt(states[2], 16);
                            }
                        } catch (NumberFormatException ex) {
                            Log.w(LOG_TAG, "error parsing location: " + ex);
                        }
                    }
                    cellLoc.setLacAndCid(lac, cid);
                    phone.notifyLocationChanged();
                }

                // Release any temporary cell lock, which could have been
                // acquired to allow a single-shot location update.
                disableSingleLocationUpdate();
                break;

            case EVENT_POLL_STATE_REGISTRATION:
            case EVENT_POLL_STATE_GPRS:
            case EVENT_POLL_STATE_OPERATOR:
            case EVENT_POLL_STATE_NETWORK_SELECTION_MODE:
                ar = (AsyncResult) msg.obj;

                handlePollStateResult(msg.what, ar);
                break;

            case EVENT_POLL_SIGNAL_STRENGTH:
                // Just poll signal strength...not part of pollState()

                cm.getSignalStrength(obtainMessage(EVENT_GET_SIGNAL_STRENGTH));
                break;

            case EVENT_NITZ_TIME:
                ar = (AsyncResult) msg.obj;

                String nitzString = (String)((Object[])ar.result)[0];
                long nitzReceiveTime = ((Long)((Object[])ar.result)[1]).longValue();

                setTimeFromNITZString(nitzString, nitzReceiveTime);
                break;

            case EVENT_SIGNAL_STRENGTH_UPDATE:
                // This is a notification from
                // CommandsInterface.setOnSignalStrengthUpdate

                ar = (AsyncResult) msg.obj;

                // The radio is telling us about signal strength changes
                // we don't have to ask it
                dontPollSignalStrength = true;

                onSignalStrengthResult(ar);
                break;

            case EVENT_SIM_RECORDS_LOADED:
                updateSpnDisplay();
                break;

            case EVENT_LOCATION_UPDATES_ENABLED:
                ar = (AsyncResult) msg.obj;

                if (ar.exception == null) {
                    cm.getVoiceRegistrationState(obtainMessage(EVENT_GET_LOC_DONE, null));
                }
                break;

            case EVENT_SET_PREFERRED_NETWORK_TYPE:
                ar = (AsyncResult) msg.obj;
                // Don't care the result, only use for dereg network (COPS=2)
                message = obtainMessage(EVENT_RESET_PREFERRED_NETWORK_TYPE, ar.userObj);
                cm.setPreferredNetworkType(mPreferredNetworkType, message);
                break;

            case EVENT_RESET_PREFERRED_NETWORK_TYPE:
                ar = (AsyncResult) msg.obj;
                if (ar.userObj != null) {
                    AsyncResult.forMessage(((Message) ar.userObj)).exception
                            = ar.exception;
                    ((Message) ar.userObj).sendToTarget();
                }
                break;

            case EVENT_GET_PREFERRED_NETWORK_TYPE:
                ar = (AsyncResult) msg.obj;

                if (ar.exception == null) {
                    mPreferredNetworkType = ((int[])ar.result)[0];
                } else {
                    mPreferredNetworkType = RILConstants.NETWORK_MODE_GLOBAL;
                }

                message = obtainMessage(EVENT_SET_PREFERRED_NETWORK_TYPE, ar.userObj);
                int toggledNetworkType = RILConstants.NETWORK_MODE_GLOBAL;

                cm.setPreferredNetworkType(toggledNetworkType, message);
                break;

            case EVENT_CHECK_REPORT_GPRS:
                if (ss != null && !isGprsConsistent(gprsState, ss.getState())) {

                    // Can't register data service while voice service is ok
                    // i.e. CREG is ok while CGREG is not
                    // possible a network or baseband side error
                    GsmCellLocation loc = ((GsmCellLocation)phone.getCellLocation());
                    EventLog.writeEvent(EventLogTags.DATA_NETWORK_REGISTRATION_FAIL,
                            ss.getOperatorNumeric(), loc != null ? loc.getCid() : -1);
                    mReportedGprsNoReg = true;
                }
                mStartedGprsRegCheck = false;
                break;

            case EVENT_RESTRICTED_STATE_CHANGED:
                // This is a notification from
                // CommandsInterface.setOnRestrictedStateChanged

                if (DBG) log("EVENT_RESTRICTED_STATE_CHANGED");

                ar = (AsyncResult) msg.obj;

                onRestrictedStateChanged(ar);
                break;

            default:
                super.handleMessage(msg);
            break;
        }
    }

    protected void setPowerStateToDesired() {
        // If we want it on and it's off, turn it on
        if (mDesiredPowerState
            && cm.getRadioState() == CommandsInterface.RadioState.RADIO_OFF) {
            cm.setRadioPower(true, null);
        } else if (!mDesiredPowerState && cm.getRadioState().isOn()) {
            // If it's on and available and we want it off gracefully
            DataConnectionTracker dcTracker = phone.mDataConnectionTracker;
            powerOffRadioSafely(dcTracker);
        } // Otherwise, we're in the desired state
    }

    @Override
    protected void hangupAndPowerOff() {
        // hang up all active voice calls
        if (phone.isInCall()) {
            phone.mCT.ringingCall.hangupIfAlive();
            phone.mCT.backgroundCall.hangupIfAlive();
            phone.mCT.foregroundCall.hangupIfAlive();
        }

        cm.setRadioPower(false, null);
    }

    protected void updateSpnDisplay() {
        int rule = phone.mIccRecords.getDisplayRule(ss.getOperatorNumeric());
        String spn = phone.mIccRecords.getServiceProviderName();
        String plmn = ss.getOperatorAlphaLong();

        // For emergency calls only, pass the EmergencyCallsOnly string via EXTRA_PLMN
        if (mEmergencyOnly && cm.getRadioState().isOn()) {
            plmn = Resources.getSystem().
                getText(com.android.internal.R.string.emergency_calls_only).toString();
            if (DBG) log("updateSpnDisplay: emergency only and radio is on plmn='" + plmn + "'");
        }

        if (rule != curSpnRule
                || !TextUtils.equals(spn, curSpn)
                || !TextUtils.equals(plmn, curPlmn)) {
            boolean showSpn = !mEmergencyOnly && !TextUtils.isEmpty(spn)
                && (rule & SIMRecords.SPN_RULE_SHOW_SPN) == SIMRecords.SPN_RULE_SHOW_SPN;
            boolean showPlmn = !TextUtils.isEmpty(plmn) &&
                (rule & SIMRecords.SPN_RULE_SHOW_PLMN) == SIMRecords.SPN_RULE_SHOW_PLMN;

            if (DBG) {
                log(String.format("updateSpnDisplay: changed sending intent" + " rule=" + rule +
                            " showPlmn='%b' plmn='%s' showSpn='%b' spn='%s'",
                            showPlmn, plmn, showSpn, spn));
            }
            Intent intent = new Intent(Intents.SPN_STRINGS_UPDATED_ACTION);
            intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
            intent.putExtra(Intents.EXTRA_SHOW_SPN, showSpn);
            intent.putExtra(Intents.EXTRA_SPN, spn);
            intent.putExtra(Intents.EXTRA_SHOW_PLMN, showPlmn);
            intent.putExtra(Intents.EXTRA_PLMN, plmn);
            phone.getContext().sendStickyBroadcast(intent);
        }

        curSpnRule = rule;
        curSpn = spn;
        curPlmn = plmn;
    }

    /**
     * Handle the result of one of the pollState()-related requests
     */
    protected void handlePollStateResult (int what, AsyncResult ar) {
        int ints[];
        String states[];

        // Ignore stale requests from last poll
        if (ar.userObj != pollingContext)
        {
           return;
        }

        if (ar.exception != null) {
            CommandException.Error err=null;
            
            /* [Bug0001532 Nusmart alex. for quicker register.] */
            if(phone.getContext().getResources().getBoolean(
                    com.android.internal.R.bool.config_radioQuickRegister)){
               pollNetworkState = false;
            }
            /* [Bug0001532 Nusmart alex. for quicker register.] */

            if (ar.exception instanceof CommandException) {
                err = ((CommandException)(ar.exception)).getCommandError();
            }

            if (err == CommandException.Error.RADIO_NOT_AVAILABLE) {
                // Radio has crashed or turned off
                cancelPollState();
                return;
            }

            if (!cm.getRadioState().isOn()) {
                // Radio has crashed or turned off
                cancelPollState();
                return;
            }

            if (err != CommandException.Error.OP_NOT_ALLOWED_BEFORE_REG_NW) {
                loge("RIL implementation has returned an error where it must succeed" +
                        ar.exception);
            }
        } else try {
            switch (what) {
                case EVENT_POLL_STATE_REGISTRATION:
                    states = (String[])ar.result;
                    int lac = -1;
                    int cid = -1;
                    int regState = -1;
                    int reasonRegStateDenied = -1;
                    int psc = -1;
                    if (states.length > 0) {
                        try {
                            regState = Integer.parseInt(states[0]);
                            if (states.length >= 3) {
                                if (states[1] != null && states[1].length() > 0) {
                                    lac = Integer.parseInt(states[1], 16);
                                }
                                if (states[2] != null && states[2].length() > 0) {
                                    cid = Integer.parseInt(states[2], 16);
                                }
                            }
                            if (states.length > 14) {
                                if (states[14] != null && states[14].length() > 0) {
                                    psc = Integer.parseInt(states[14], 16);
                                }
                            }
                        } catch (NumberFormatException ex) {
                            loge("error parsing RegistrationState: " + ex);
                        }
                    }

                    mGsmRoaming = regCodeIsRoaming(regState);
                    newSS.setState (regCodeToServiceState(regState));

                    if (regState == 10 || regState == 12 || regState == 13 || regState == 14) {
                        mEmergencyOnly = true;
                    } else {
                        mEmergencyOnly = false;
                    }

                    // LAC and CID are -1 if not avail
                    newCellLoc.setLacAndCid(lac, cid);
                    newCellLoc.setPsc(psc);
                break;

                case EVENT_POLL_STATE_GPRS:
                    states = (String[])ar.result;

                    int type = 0;
                    regState = -1;
                    mNewReasonDataDenied = -1;
                    mNewMaxDataCalls = 1;
                    if (states.length > 0) {
                        try {
                            regState = Integer.parseInt(states[0]);

                            // states[3] (if present) is the current radio technology
                            if (states.length >= 4 && states[3] != null) {
                                type = Integer.parseInt(states[3]);
                            }
                            if ((states.length >= 5 ) && (regState == 3)) {
                                mNewReasonDataDenied = Integer.parseInt(states[4]);
                            }
                            if (states.length >= 6) {
                                mNewMaxDataCalls = Integer.parseInt(states[5]);
                            }
                        } catch (NumberFormatException ex) {
                            loge("error parsing GprsRegistrationState: " + ex);
                        }
                    }
                    newGPRSState = regCodeToServiceState(regState);
                    mDataRoaming = regCodeIsRoaming(regState);
                    mNewRadioTechnology = type;
                    newSS.setRadioTechnology(type);
                break;

                case EVENT_POLL_STATE_OPERATOR:
                    String opNames[] = (String[])ar.result;

                    if (opNames != null && opNames.length >= 3) {
                    	// +[bug0001494 android_team jackyyang 20120314] fix senario of "no plmn"
                    	String operatorId = phone.mIccRecords.getOperatorNumeric();
                    	
                    	if(opNames[2]!=null && (opNames[0]==null || (operatorId!=null&&operatorId.equals(opNames[0])))) {
                    		String operatorName = findOperatorNameForChina(operatorId);
                    		if(operatorName == null) {
                    			operatorName = findOperatorName(operatorId,sOperatorMap);
                    		}
                    		if(operatorName!=null) {
                    			opNames[0] = operatorName;
                    		}
                    	}
                    	// -[bug0001494 android_team jackyyang 20120314]
                        newSS.setOperatorName (
                                opNames[0], opNames[1], opNames[2]);
                    }
                break;

                case EVENT_POLL_STATE_NETWORK_SELECTION_MODE:
                    ints = (int[])ar.result;
                    newSS.setIsManualSelection(ints[0] == 1);
                break;
            }

        } catch (RuntimeException ex) {
            loge("Exception while polling service state. Probably malformed RIL response." + ex);
        }

        pollingContext[0]--;

        if (pollingContext[0] == 0) {
           /* [Bug0001532 Nusmart alex. for quicker register.] */
           if(phone.getContext().getResources().getBoolean(
                    com.android.internal.R.bool.config_radioQuickRegister)){
              pollingContext = null;
              pollNetworkState = false;
           }
           /* [Bug0001532 Nusmart alex. for quicker register.] */
            /**
             *  Since the roaming states of gsm service (from +CREG) and
             *  data service (from +CGREG) could be different, the new SS
             *  is set roaming while either one is roaming.
             *
             *  There is an exception for the above rule. The new SS is not set
             *  as roaming while gsm service reports roaming but indeed it is
             *  not roaming between operators.
             */
            boolean roaming = (mGsmRoaming || mDataRoaming);
            if (mGsmRoaming && !isRoamingBetweenOperators(mGsmRoaming, newSS)) {
                roaming = false;
            }
            newSS.setRoaming(roaming);
            newSS.setEmergencyOnly(mEmergencyOnly);
            if(DBG) log("@@@Nusmart---handlePollStateResult---@@@");
            pollStateDone();
        }
    }
    // +[bug0001494 android_team jackyyang 20120314] fix senario of "no plmn"
    private String findOperatorNameForChina(String operatorId) {
    	String operatorName;
    	if(phone.getContext().getResources().getConfiguration().locale.equals(Locale.SIMPLIFIED_CHINESE)) {
    		operatorName = findOperatorName(operatorId,sOperatorMapForChinaZh);
    	} else {
    		operatorName = findOperatorName(operatorId, sOperatorMapForChinaEn);
    	}
    	return operatorName;
    }
    
    
    private String findOperatorName(String operatorId, String[][] operatorMap) {
    	for(int i=0;i<operatorMap.length;i++) {
    		if(operatorMap[i][0].equals(operatorId)) {
    			return operatorMap[i][1];
    		}
    	}
    	return null;
    }
    private static String sOperatorMapForChinaEn[][] = {
    	{ "46000", "China Mobile"			},
	    { "46001", "China Unicom"		},
	    { "46002", "China Mobile"			},
	    { "46003", "China Telecom"		},
	    { "46007", "China Mobile"			},
    };
    private static String sOperatorMapForChinaZh[][] = {
    	{ "46000", "中国移动"			},
	    { "46001", "中国联通"		},
	    { "46002", "中国移动"			},
	    { "46003", "中国电信"		},
	    { "46007", "中国移动"			},
    };
    
    private static String sOperatorMap[][] = {
    	    { "20201", "GR COSMOTE"			},
    	    { "20205", "vodafone GR"			},
    	    { "20209", "Q-TELECOM"			},
    	    { "20210", "TIM GR"			},
    	    { "20404", "vodafone NL"			},
    	    { "20408", "NL KPN"			},
    	    { "20412", "NL Telfort"			},
    	    { "20416", "T-Mobile NL"			},
    	    { "20420", "Orange NL"			},
    	    { "20601", "BEL PROXIMUS"		},
    	    { "20610", "B Mobistar"			},
    	    { "20620", "BASE"			},
    	    { "20801", "Orange F"			},
    	    { "20802", "F - Contact"			},
    	    { "20810", "F SFR"			},
    	    { "20811", "SFR Home 3G" 		},
    	    { "20813", "F - Contact"			},
    	    { "20820", "BYTEL"			},
    	    { "20888", "F - Contact"			},
    	    { "21210", "Monaco"			},
    	    { "21303", "STA-MOBILAND"		},
    	    { "21401", "vodafone ES"			},
    	    { "21402", "movistar"			},
    	    { "21403", "Orange"			},
    	    { "21404", "Yoigo"			},
    	    { "21407", "movistar"			},
    	    { "21601", "pannon 3G"			},
    	    { "21630", "T-Mobile H"			},
    	    { "21670", "vodafone HU"			},
    	    { "21803", "ERONET"			},
    	    { "21805", "m:tel"			},
    	    { "21890", "BH GSMBIH"			},
    	    { "21901", "T-Mobile HR"			},
    	    { "21902", "TELE2"			},
    	    { "21910", "HR VIP"			},
    	    { "22001", "Telenor"			},
    	    { "22002", "ProMonte"			},
    	    { "22003", "MTS"				},
    	    { "22004", "T-Mobile CG"			},
    	    { "22005", "Vip SRB"			},
    	    { "22201", "TIM"				},
    	    { "22210", "vodafone IT"			},
    	    { "22288", "I WIND"			},
    	    { "22298", "Blu SpA"			},
    	    { "22299", "3 ITA"			},
    	    { "22601", "Vodafone RO"			},
    	    { "22603", "RO COSMOTE"			},
    	    { "22605", "RO Digi.Mobil"		},
    	    { "22610", "RO ORANGE"			},
    	    { "22801", "Swisscom"			},
    	    { "22802", "sunrise"			},
    	    { "22803", "orange CH"			},
    	    { "22807", "In&Phone"			},
    	    { "22808", "T2"				},
    	    { "23001", "T-Mobile CZ"			},
    	    { "23002", "EUROTEL - CZ"		},
    	    { "23003", "Vodafone CZ"			},
    	    { "23101", "Orange SK"			},
    	    { "23102", "T-Mobile SK"			},
    	    { "23106", "O2-SK"			},
    	    { "23201", "A1"				},
    	    { "23203", "T-Mobile Austria"		},
    	    { "23205", "Orange A"			},
    	    { "23207", "tele - ring"			},
    	    { "23210", "3 AT"			},
    	    { "23212", "Orange A"			},
    	    { "23401", "UK01"			},
    	    { "23403", "ATL-VOD"			},
    	    { "23407", "C&W UK"			},
    	    { "23409", "PMN UK"			},
    	    { "23410", "O2 - UK"			},
    	    { "23415", "vodafone UK"			},
    	    { "23416", "Opal UK"			},
    	    { "23419", "PMN UK"			},
    	    { "23420", "3 UK"			},
    	    { "23430", "T-Mobile UK"			},
    	    { "23431", "T-Mobile UK"			},
    	    { "23432", "T-Mobile UK"			},
    	    { "23433", "Orange"			},
    	    { "23450", "JT-Wave"			},
    	    { "23455", "C&W"				},
    	    { "23458", "MANX"			},
    	    { "23801", "TDC MOBIL"			},
    	    { "23802", "Telenor DK"			},
    	    { "23806", "3 DK"			},
    	    { "23820", "TELIA DK"			},
    	    { "23830", "Orange"			},
    	    { "23877", "Telenor DK"			},
    	    { "24001", "TELIA S"			},
    	    { "24002", "3 SE"			},
    	    { "24004", "Telenor SE"			},
    	    { "24005", "Sweden3G"			},
    	    { "24007", "IQ"				},
    	    { "24008", "Telenor SE"			},
    	    { "24010", "Spring"			},
    	    { "24201", "N Telenor"			},
    	    { "24202", "NetCom"			},
    	    { "24203", "MTU"				},
    	    { "24205", "NetworkN"			},
    	    { "24403", "dna"				},
    	    { "24405", "elisa"			},
    	    { "24409", "FINNET"			},
    	    { "24412", "dna"				},
    	    { "24414", "FI AMT"			},
    	    { "24491", "SONERA"			},
    	    { "24601", "OMNITEL LT"			},
    	    { "24602", "BITE GSM"			},
    	    { "24603", "TELE2"			},
    	    { "24701", "LMT"				},
    	    { "24702", "TELE2"			},
    	    { "24705", "BITE LV"			},
    	    { "24801", "EMT"				},
    	    { "24802", "elisa"			},
    	    { "24803", "TELE2"			},
    	    { "25001", "MTS"				},
    	    { "25002", "MegaFon"			},
    	    { "25003", "NCC"				},
    	    { "25004", "RUS_SCN"			},
    	    { "25005", "SCS"				},
    	    { "25007", "SMARTS"			},
    	    { "25010", "DTC"				},
    	    { "25011", "Orensot"			},
    	    { "25012", "Far East"			},
    	    { "25013", "KUGSM"			},
    	    { "25015", "SMARTS"			},
    	    { "25016", "NTC"				},
    	    { "25017", "Utel"			},
    	    { "25019", "INDIGO"			},
    	    { "25020", "TELE2"			},
    	    { "25028", "Beeline"			},
    	    { "25035", "MOTIV"			},
    	    { "25039", "Utel"			},
    	    { "25044", "NC-GSM"			},
    	    { "25091", "Sonic Duo"			},
    	    { "25092", "Primtel"			},
    	    { "25093", "JSC Telecom XXI"		},
    	    { "25099", "Beeline"			},
    	    { "25501", "MTS UKR"			},
    	    { "25502", "Beeline UA"			},
    	    { "25503", "UA-KYIVSTAR"			},
    	    { "25505", "GT"				},
    	    { "25506", "life:)"			},
    	    { "25701", "BY VELCOM"			},
    	    { "25702", "MTS BY"			},
    	    { "25704", "BeST BY"			},
    	    { "25901", "Orange MD"			},
    	    { "25902", "MD MOLDCELL"			},
    	    { "25904", "MDA EVENTIS"			},
    	    { "26001", "Plus"			},
    	    { "26002", "Era"				},
    	    { "26003", "Orange PL"			},
    	    { "26006", "Play"			},
    	    { "26201", "Telekom.de"			},
    	    { "26202", "Vodafone.de"			},
    	    { "26203", "E-Plus"			},
    	    { "26207", "o2 - de"			},
    	    { "26208", "o2 - de"			},
    	    { "26213", "Mobilcom"			},
    	    { "26601", "GIBTEL GSM"			},
    	    { "26801", "vodafone P"			},
    	    { "26803", "OPTIMUS"			},
    	    { "26806", "TMN"				},
    	    { "27001", "LUXGSM"			},
    	    { "27077", "TANGO"			},
    	    { "27099", "L Orange-LU"			},
    	    { "27201", "vodafone IE"			},
    	    { "27202", "O2 - IRL"			},
    	    { "27203", "METEOR"			},
    	    { "27205", "3 IRL"			},
    	    { "27401", "Siminn"			},
    	    { "27402", "Vodafone"			},
    	    { "27403", "Og Vodafone"			},
    	    { "27404", "Viking"			},
    	    { "27407", "IS-IceCell"			},
    	    { "27408", "On-waves"			},
    	    { "27411", "NOVA IS"			},
    	    { "27601", "AMC - AL"			},
    	    { "27602", "vodafone AL"			},
    	    { "27603", "EAGLE AL"			},
    	    { "27801", "vodafone MT"			},
    	    { "27821", "go mobile"			},
    	    { "27877", "3GT MT"			},
    	    { "28001", "CYTAMOBILE-VODAFONE"		},
    	    { "28010", "MTN"				},
    	    { "28201", "GEOCELL"			},
    	    { "28202", "MAGTI"			},
    	    { "28203", "GEO 03"			},
    	    { "28204", "BEELINE GE"			},
    	    { "28301", "Beeline AM"			},
    	    { "28305", "ARM MTS"			},
    	    { "28401", "M-Tel BG"			},
    	    { "28403", "vivatel"			},
    	    { "28405", "BG GLOBUL"			},
    	    { "28601", "TR TURKCELL"			},
    	    { "28602", "VODAFONE TR"			},
    	    { "28603", "AVEA"			},
    	    { "28604", "AYCELL"			},
    	    { "28801", "FT-GSM"			},
    	    { "28802", "VODAFONE FO"			},
    	    { "29001", "TELE Greenland"		},
    	    { "29266", "SMT"				},
    	    { "29340", "Si.mobil"			},
    	    { "29341", "MOBITEL"			},
    	    { "29364", "T-2"				},
    	    { "29370", "SI TUSMOBIL"			},
    	    { "29401", "T-Mobile MK"			},
    	    { "29402", "MKD COSMOFON"		},
    	    { "29403", "Vip MKD"			},
    	    { "29501", "SwisscomFL"			},
    	    { "29502", "Orange FL"			},
    	    { "29505", "FL1"				},
    	    { "29577", "L1 TANGO"			},
    	    { "30200", "Bell Mobility"		},
    	    { "30222", "TELUS"			},
    	    { "30235", "CANFN"			},
    	    { "30237", "Fido"			},
    	    { "30238", "DMTS GSM"			},
    	    { "30261", "Bell"			},
    	    { "30264", "Bell Mobility"		},
    	    { "30272", "Rogers Wireless"		},
    	    { "30801", "SPM AMERIS"			},
    	    { "30802", "PROSOD"			},
    	    { "31000", "Sprint PCS"			},
    	    { "31001", "Cellnet"			},
    	    { "31002", "UnionTel"			},
    	    { "31003", "Centennial Communications"	},
    	    { "31004", "Cellular One"		},
    	    { "31005", "DIGICEL"			},
    	    { "31007", "Highland Cellular"		},
    	    { "31008", "Corr Wireless"		},
    	    { "31010", "US PLATEAU"			},
    	    { "31011", "Wireless 2000"		},
    	    { "31015", "AT&T"			},
    	    { "31016", "T-Mobile"			},
    	    { "31017", "AT&T"			},
    	    { "31018", "West Central"		},
    	    { "31019", "USA Dutch Harbor"		},
    	    { "31020", "T-Mobile"			},
    	    { "31021", "T-Mobile"			},
    	    { "31022", "T-Mobile"			},
    	    { "31023", "T-Mobile"			},
    	    { "31024", "T-Mobile"			},
    	    { "31025", "T-Mobile"			},
    	    { "31026", "T-Mobile"			},
    	    { "31027", "T-Mobile"			},
    	    { "31030", "CENT USA"			},
    	    { "31031", "T-Mobile"			},
    	    { "31032", "Cell"			},
    	    { "31033", "Cellular One"		},
    	    { "31034", "WestLink"			},
    	    { "31035", "Carolina"			},
    	    { "31038", "AT&T"			},
    	    { "31039", "Cell1ET"			},
    	    { "31040", "USA iCAN"			},
    	    { "31041", "AT&T"			},
    	    { "31042", "CBW"				},
    	    { "31045", "NECCI"			},
    	    { "31046", "USA SIMMETRY"		},
    	    { "31047", "USA DOCOMO PACIFIC"		},
    	    { "31049", "T-Mobile"			},
    	    { "31053", "lowa Wireless USA"		},
    	    { "31056", "Cell One"			},
    	    { "31057", "Chinook"			},
    	    { "31058", "T-Mobile"			},
    	    { "31059", "ROAMING"			},
    	    { "31061", "Epic Touch"			},
    	    { "31063", "USA AmeriLink"		},
    	    { "31064", "USA AE Airadigm"		},
    	    { "31065", "Jasper"			},
    	    { "31066", "T-Mobile"			},
    	    { "31067", "Wireless 2000 PCS"		},
    	    { "31068", "NPI"				},
    	    { "31069", "IMMIX"			},
    	    { "31070", "USABIGFOOT"			},
    	    { "31071", "USA ASTAC"			},
    	    { "31074", "TELEMETRIX"			},
    	    { "31076", "PTSI"			},
    	    { "31077", "i- wireless"			},
    	    { "31078", "AirLink PCS"			},
    	    { "31079", "USA Pinpoint"		},
    	    { "31080", "Corr Wireless"		},
    	    { "31087", "PACE"			},
    	    { "31088", "USAACSI"			},
    	    { "31089", "USA Unicel"			},
    	    { "31090", "TXCELL"			},
    	    { "31091", "FCSI"			},
    	    { "31095", "USA XIT Wireless"		},
    	    { "31098", "AT&T"			},
    	    { "31100", "Mid-Tex"			},
    	    { "31103", "Indigo"			},
    	    { "31104", "USA - Commnet"		},
    	    { "31107", "EASTER"			},
    	    { "31108", "Pine Cellular"		},
    	    { "31109", "USASXLP"			},
    	    { "31111", "High Plains"			},
    	    { "31113", "Cell One Amarillo"		},
    	    { "31114", "Sprocket"			},
    	    { "31117", "PetroCom"			},
    	    { "31118", "AT&T"			},
    	    { "31119", "USAC1ECI"			},
    	    { "31121", "FARMERS"			},
    	    { "31125", "iCAN_GSM"			},
    	    { "31126", "SLP Cellular"		},
    	    { "31131", "Lamar Cellular"		},
    	    { "31133", "BTW"				},
    	    { "31601", "Nextel"			},
    	    { "33011", "PRClaro"			},
    	    { "33211", "Blue Sky"			},
    	    { "33402", "TELCEL GSM"                  },
    	    { "33403", "movistar"			},
    	    { "33420", "TELCEL"			},
    	    { "33430", "APUP-PCS"			},
    	    { "33805", "DIGICEL"			},
    	    { "33807", "Claro JAM"                   },
    	    { "33818", "C&W"				},
    	    { "33870", "Claro JAM"			},
    	    { "34001", "F-Orange"			},
    	    { "34002", "ONLY"			},
    	    { "34008", "MIO"				},
    	    { "34020", "DigicelF"			},
    	    { "34260", "C&W"				},
    	    { "34275", "DIGICEL"			},
    	    { "34430", "APUA-PCS"			},
    	    { "34492", "C&W"				},
    	    { "34493", "Cingular"			},
    	    { "34614", "LIME"			},
    	    { "34857", "CCT Boatphone"		},
    	    { "35000", "CellularOne"			},
    	    { "35001", "Telecom"			},
    	    { "35002", "MOBILITY"			},
    	    { "35010", "Cingular"			},
    	    { "35211", "C&W"				},
    	    { "35230", "DIGICEL"			},
    	    { "35486", "LIME"			},
    	    { "35611", "LIME"			},
    	    { "35811", "C&W"				},
    	    { "35850", "DIGICEL"			},
    	    { "36011", "C&W"				},
    	    { "36070", "DIGICEL"			},
    	    { "36251", "Telcell GSM"			},
    	    { "36269", "CT GSM"			},
    	    { "36301", "SETAR GSM"			},
    	    { "36320", "DIGICEL"			},
    	    { "36439", "BaTelCell"			},
    	    { "36584", "C&W"				},
    	    { "36611", "C&W"				},
    	    { "36620", "Cingular"			},
    	    { "36801", "CUBACEL"			},
    	    { "37001", "orange"			},
    	    { "37002", "ClaroDOM"			},
    	    { "37201", "COMCEL"			},
    	    { "37412", "TSTT"			},
    	    { "37635", "C&W"				},
    	    { "40001", "AZERCELL GSM"		},
    	    { "40002", "BAKCELL GSM 2000"		},
    	    { "40004", "AZ Nar"			},
    	    { "40101", "Beeline KZ"			},
    	    { "40102", "KZ KCELL"			},
    	    { "40177", "NEO-KZ"			},
    	    { "40211", "BT B-Mobile"			},
    	    { "40277", "TASHICELL"			},
    	    { "40401", "Vodafone IN"			},
    	    { "40402", "AirTel"			},
    	    { "40403", "AirTel"			},
    	    { "40404", "!DEA"			},
    	    { "40405", "Vodafone IN"			},
    	    { "40407", "!DEA"			},
    	    { "40409", "Reliance"			},
    	    { "40410", "AirTel"			},
    	    { "40411", "Vodafone IN"			},
    	    { "40412", "!DEA"			},
    	    { "40413", "Vodafone IN"			},
    	    { "40414", "!DEA"			},
    	    { "40415", "Vodafone IN"			},
    	    { "40416", "AirTel"			},
    	    { "40417", "Aircel"			},
    	    { "40418", "Reliance"			},
    	    { "40419", "!DEA"			},
    	    { "40420", "Vodafone IN"			},
    	    { "40421", "Loop"			},
    	    { "40422", "!DEA"			},
    	    { "40424", "!DEA"			},
    	    { "40425", "Aircel"			},
    	    { "40427", "Vodafone IN"			},
    	    { "40428", "Aircel"			},
    	    { "40429", "Aircel"			},
    	    { "40430", "Vodafone IN"			},
    	    { "40431", "AirTel"			},
    	    { "40433", "Aircel"			},
    	    { "40434", "CellOne"			},
    	    { "40435", "Aircel"			},
    	    { "40436", "Reliance"			},
    	    { "40437", "Aircel"			},
    	    { "40438", "CellOne"			},
    	    { "40440", "AirTel"			},
    	    { "40441", "Aircel"			},
    	    { "40442", "Aircel"			},
    	    { "40443", "Vodafone IN"			},
    	    { "40444", "!DEA"			},
    	    { "40445", "AirTel"			},
    	    { "40446", "Vodafone IN"			},
    	    { "40449", "AirTel"			},
    	    { "40450", "Reliance"			},
    	    { "40451", "CellOne"			},
    	    { "40452", "Reliance"			},
    	    { "40453", "CellOne"			},
    	    { "40454", "CellOne"			},
    	    { "40455", "CellOne"			},
    	    { "40456", "!DEA"			},
    	    { "40457", "CellOne"			},
    	    { "40458", "CellOne"			},
    	    { "40459", "CellOne"			},
    	    { "40460", "Vodafone IN"			},
    	    { "40462", "CellOne"			},
    	    { "40464", "CellOne"			},
    	    { "40466", "CellOne"			},
    	    { "40467", "Reliance"			},
    	    { "40468", "DOLPHIN"			},
    	    { "40469", "DOLPHIN"			},
    	    { "40470", "AirTel"			},
    	    { "40471", "CellOne"			},
    	    { "40472", "CellOne"			},
    	    { "40473", "CellOne"			},
    	    { "40474", "CellOne"			},
    	    { "40475", "CellOne"			},
    	    { "40476", "CellOne"			},
    	    { "40477", "CellOne"			},
    	    { "40478", "!DEA"			},
    	    { "40479", "CellOne"			},
    	    { "40480", "CellOne"			},
    	    { "40481", "CellOne"			},
    	    { "40482", "!DEA"			},
    	    { "40483", "Reliance"			},
    	    { "40484", "Vodafone IN"			},
    	    { "40485", "Reliance"			},
    	    { "40486", "Vodafone IN"			},
    	    { "40487", "!DEA"			},
    	    { "40488", "Vodafone IN"			},
    	    { "40489", "!DEA"			},
    	    { "40490", "AirTel"			},
    	    { "40491", "Aircel"			},
    	    { "40492", "AirTel"			},
    	    { "40493", "AirTel"			},
    	    { "40494", "AirTel"			},
    	    { "40495", "AirTel"			},
    	    { "40496", "AirTel"			},
    	    { "40497", "AirTel"			},
    	    { "40498", "AirTel"			},
    	    { "40501", "Reliance"			},
    	    { "40504", "Reliance"			},
    	    { "40505", "Reliance"			},
    	    { "40506", "Reliance"			},
    	    { "40507", "Reliance"			},
    	    { "40509", "Reliance"			},
    	    { "40510", "Reliance"			},
    	    { "40511", "Reliance"			},
    	    { "40513", "Reliance"			},
    	    { "40515", "Reliance"			},
    	    { "40518", "Reliance"			},
    	    { "40519", "Reliance"			},
    	    { "40520", "Reliance"			},
    	    { "40521", "Reliance"			},
    	    { "40522", "Reliance"			},
    	    { "40525", "TATA DOCOMO"			},
    	    { "40527", "TATA DOCOMO"			},
    	    { "40529", "TATA DOCOMO"			},
    	    { "40530", "TATA DOCOMO"			},
    	    { "40531", "TATA DOCOMO"			},
    	    { "40532", "TATA DOCOMO"			},
    	    { "40534", "TATA DOCOMO"			},
    	    { "40535", "TATA DOCOMO"			},
    	    { "40536", "TATA DOCOMO"			},
    	    { "40537", "TATA DOCOMO"			},
    	    { "40538", "TATA DOCOMO"			},
    	    { "40539", "TATA DOCOMO"			},
    	    { "40541", "TATA DOCOMO"			},
    	    { "40542", "TATA DOCOMO"			},
    	    { "40543", "TATA DOCOMO"			},
    	    { "40544", "TATA DOCOMO"			},
    	    { "40545", "TATA DOCOMO"			},
    	    { "40546", "TATA DOCOMO"			},
    	    { "40547", "TATA DOCOMO"			},
    	    { "40550", "Reliance"			},
    	    { "40551", "AirTel"			},
    	    { "40552", "AirTel"			},
    	    { "40553", "AirTel"			},
    	    { "40554", "AirTel"			},
    	    { "40555", "AirTel"			},
    	    { "40556", "AirTel"			},
    	    { "40566", "Vodafone IN"			},
    	    { "40567", "Vodafone IN"			},
    	    { "40570", "!DEA"			},
    	    { "40575", "Vodafone IN"			},
    	    { "40580", "Aircel"			},
    	    { "40585", "!DEA"			},
    	    { "41001", "Mobilink"			},
    	    { "41003", "PK-UFONE"			},
    	    { "41004", "ZONG"			},
    	    { "41006", "Telenor PK"			},
    	    { "41007", "WaridTel"			},
    	    { "41201", "AF AWCC"			},
    	    { "41220", "ROSHAN"			},
    	    { "41240", "MTN AF"			},
    	    { "41250", "Etisalat"			},
    	    { "41301", "Mobitel"			},
    	    { "41302", "DIALOG"			},
    	    { "41303", "Tigo"			},
    	    { "41305", "SIR AIRTEL"			},
    	    { "41308", "Hutch"			},
    	    { "41401", "MPTGSM"			},
    	    { "41501", "alfa"			},
    	    { "41503", "RL MTC Lebanon"		},
    	    { "41505", "OM"				},
    	    { "41601", "zain JO"			},
    	    { "41603", "UMNIAH"			},
    	    { "41677", "Orange JO"			},
    	    { "41701", "SYRIATEL"			},
    	    { "41702", "MTN"				},
    	    { "41709", "SYR MOBILE SYR"		},
    	    { "41800", "ASIACELL"			},
    	    { "41802", "SanaTel"			},
    	    { "41805", "ASIACELL"			},
    	    { "41808", "SanaTel"			},
    	    { "41820", "zain IQ"			},
    	    { "41830", "IRAQNA"			},
    	    { "41840", "KOREK"			},
    	    { "41902", "Zain KW"			},
    	    { "41903", "WATANIYA"			},
    	    { "41904", "VIVA"			},
    	    { "42001", "STC"				},
    	    { "42003", "mobily"			},
    	    { "42007", "EAE"				},
    	    { "42101", "SabaFon"			},
    	    { "42102", "MTN"				},
    	    { "42170", "YemYY"			},
    	    { "42202", "OMAN MOBILE"			},
    	    { "42203", "nawras"			},
    	    { "42402", "ETISALAT"			},
    	    { "42403", "du"				},
    	    { "42501", "ORANGE"			},
    	    { "42502", "Cellcom"			},
    	    { "42503", "IL Pelephone"		},
    	    { "42505", "JAWWAL"			},
    	    { "42506", "WM"				},
    	    { "42601", "BATELCO"			},
    	    { "42602", "zain BH"			},
    	    { "42701", "Qtel"			},
    	    { "42702", "vodafone"			},
    	    { "42888", "UNTLMN"			},
    	    { "42899", "MobiCom"			},
    	    { "42901", "NTC"				},
    	    { "43211", "IR-TCI"			},
    	    { "43214", "IR KISH"			},
    	    { "43219", "MTCE"			},
    	    { "43232", "Taliya"			},
    	    { "43235", "MTN Irancell"		},
    	    { "43401", "Buztel"			},
    	    { "43404", "Beeline UZ"			},
    	    { "43405", "Ucell"			},
    	    { "43407", "UZB MTS"			},
    	    { "43601", "Somoncom"			},
    	    { "43602", "INDIGO-T"			},
    	    { "43603", "TJK MLT"			},
    	    { "43604", "Babilon-M"			},
    	    { "43605", "BEELINE TJ"			},
    	    { "43612", "INDIGO-3G"			},
    	    { "43701", "BITEL KGZ"			},
    	    { "43705", "MEGACOM"			},
    	    { "43709", "O!"				},
    	    { "43801", "MTS TM"			},
    	    { "44000", "JPN EMOBILE"			},
    	    { "44010", "NTT DOCOMO"			},
    	    { "44020", "SoftBank"			},
    	    { "45002", "KR KTF"			},
    	    { "45005", "SKTelecom"			},
    	    { "45008", "KR KTF"			},
    	    { "45201", "VN MOBIFONE"			},
    	    { "45202", "VINAPHONE"			},
    	    { "45204", "VIETTEL"			},
    	    { "45205", "VNMOBILE"			},
    	    { "45400", "CSL"				},
    	    { "45401", "NEW WORLD"			},
    	    { "45402", "CSL"				},
    	    { "45403", "3 HK"			},
    	    { "45404", "3(2G)"			},
    	    { "45406", "SmarToneVodafone"		},
    	    { "45410", "CSL"	      			},
    	    { "45412", "China Mobile HK"		},
    	    { "45415", "454-15"			},
    	    { "45416", "PCCW"			},
    	    { "45418", "CSL"				},
    	    { "45419", "PCCW"			},
    	    { "45429", "PCCW"			},
    	    { "45500", "SmarTone"			},
    	    { "45501", "CTM"				},
    	    { "45502", "Macau" 			},
    	    { "45503", "Hutchison MAC"		},
    	    { "45504", "CTM"				},
    	    { "45505", "3 Macau"			},
    	    { "45601", "MOBITEL -KHM"		},
    	    { "45602", "TMIC"			},
    	    { "45604", "CADCOMMS"			},
    	    { "45605", "STARCELL"			},
    	    { "45606", "SMART"			},
    	    { "45608", "Metfone"			},
    	    { "45618", "mfone"			},
    	    { "45701", "LAO GSM"			},
    	    { "45702", "ETLMNW"			},
    	    { "45703", "LATMOBIL"			},
    	    { "45708", "TIGO"			},
    	    { "46000", "China Mobile"			},
    	    { "46001", "China Unicom"		},
    	    { "46002", "China Mobile"			},
    	    { "46003", "China Telecom"		},
    	    { "46007", "China Mobile"			},
    	    { "46601", "Far EasTone"			},
    	    { "46605", "APBW"			},
    	    { "46606", "TUNTEX"			},
    	    { "46668", "ACeS"			},
    	    { "46688", "KGT-Online"			},
    	    { "46689", "VIBO"			},
    	    { "46692", "Chunghwa"			},
    	    { "46693", "TWN MOBITAI"			},
    	    { "46697", "TW Mobile"			},
    	    { "46699", "TransAsia"			},
    	    { "46703", "SUNNET"			},
    	    { "47001", "GrameemPhone"		},
    	    { "47002", "AKTEL"			},
    	    { "47003", "Banglalink"			},
    	    { "47004", "bMobile"			},
    	    { "47007", "WARID BD"			},
    	    { "47019", "Mobile 2000"			},
    	    { "47201", "MV DHIMOBILE"		},
    	    { "47202", "WATANIYA"			},
    	    { "50200", "TIME3G"			},
    	    { "50201", "TIME3G"			},
    	    { "50212", "MY MAXIS"			},
    	    { "50213", "CELCOM"			},
    	    { "50216", "DiGi"			},
    	    { "50217", "ADAM"			},
    	    { "50218", "U MOBILE"			},
    	    { "50219", "CELCOM"			},
    	    { "50501", "Telstra Mobile"		},
    	    { "50502", "YES OPTUS"			},
    	    { "50503", "vodafone AU"			},
    	    { "50506", "TELSTRA"			},
    	    { "50508", "One.Tel"			},
    	    { "50510", "Norfolk Telecom"		},
    	    { "50511", "Telstra"			},
    	    { "51000", "ACeS"			},
    	    { "51001", "INDOSAT"			},
    	    { "51003", "StarOne"			},
    	    { "51008", "axis"			},
    	    { "51010", "TELKOMSEL"			},
    	    { "51011", "XL"				},
    	    { "51021", "INDOSAT"			},
    	    { "51028", "Mobile8"			},
    	    { "51051", "Mobile-8"			},
    	    { "51089", "3"				},
    	    { "51099", "Esia"			},
    	    { "51402", "TT"				},
    	    { "51501", "ISLACOM"			},
    	    { "51502", "GLOBE"			},
    	    { "51503", "SMART"			},
    	    { "51505", "SUN"				},
    	    { "51511", "ACeS"			},
    	    { "51518", "CURE"			},
    	    { "52000", "Hutch"			},
    	    { "52001", "TH GSM"			},
    	    { "52002", "CAT CDMA"			},
    	    { "52015", "ACT-1900"			},
    	    { "52018", "DTAC"			},
    	    { "52020", "ACeS"			},
    	    { "52023", "GSM 1800"			},
    	    { "52099", "TRUE"			},
    	    { "52501", "SingTel"			},
    	    { "52502", "SingTel-G18"			},
    	    { "52503", "M1-3GSM"			},
    	    { "52504", "SGP-M1-3GSM"			},
    	    { "52505", "STARHUB"			},
    	    { "52802", "b-mobile"			},
    	    { "52811", "DSTCom"			},
    	    { "53001", "vodafone NZ"			},
    	    { "53002", "TNZ"				},
    	    { "53004", "NextG NZ"			},
    	    { "53005", "Telecom NZ"			},
    	    { "53024", "NZ Comms"			},
    	    { "53701", "BMobile"			},
    	    { "53730", "Digicel PNG"			},
    	    { "53901", "U-CALL"			},
    	    { "53988", "Digicel"			},
    	    { "54001", "BREEZE"			},
    	    { "54100", "ACeS"			},
    	    { "54101", "SMILE"			},
    	    { "54105", "DIGICEL"			},
    	    { "54201", "FJ VODAFONE"			},
    	    { "54411", "BSKY"			},
    	    { "54509", "KL-Frigate"			},
    	    { "54601", "MOBILIS"			},
    	    { "54720", "VINI"			},
    	    { "54801", "KOKANET"			},
    	    { "54927", "Samoatel GO"			},
    	    { "55001", "FSM Telecom"			},
    	    { "55280", "PLWPMC"			},
    	    { "60201", "MobiNiL"			},
    	    { "60202", "vodafone EG"			},
    	    { "60203", "etisalat"			},
    	    { "60301", "Mobilis"			},
    	    { "60302", "Djezzy"			},
    	    { "60303", "NEDJMA"			},
    	    { "60400", "MEDITEL"			},
    	    { "60401", "IAM"				},
    	    { "60501", "Orange"			},
    	    { "60502", "TUNTEL"			},
    	    { "60503", "TUNSIANA"			},
    	    { "60600", "Libyana"			},
    	    { "60601", "Libya Al Madar"		},
    	    { "60701", "GAMCEL"			},
    	    { "60702", "AFRICELL"			},
    	    { "60703", "GMCOMIUM"			},
    	    { "60801", "ALIZE"			},
    	    { "60802", "SENTEL"			},
    	    { "60901", "MATTEL"			},
    	    { "60910", "MAURITEL"			},
    	    { "61001", "MALITEL ML"			},
    	    { "61002", "ORANGE ML"			},
    	    { "61101", "Orange GN"			},
    	    { "61102", "LAGUI"			},
    	    { "61104", "GNAreeba"			},
    	    { "61105", "GINCL"			},
    	    { "61201", "CORA"			},
    	    { "61202", "ACELL-CI"			},
    	    { "61203", "Orange CI"			},
    	    { "61204", "KoZ"				},
    	    { "61205", "MTN CI"			},
    	    { "61302", "BF Celtel"			},
    	    { "61401", "SAHELCOM"			},
    	    { "61402", "CELTEL"			},
    	    { "61403", "TELECEL"			},
    	    { "61404", "Orange NE"			},
    	    { "61501", "TOGO CEL"			},
    	    { "61503", "TELCEL"			},
    	    { "61601", "LIBERCOM"			},
    	    { "61602", "TELECEL BENIN"		},
    	    { "61603", "BJ MTN"			},
    	    { "61604", "BBCOM"			},
    	    { "61605", "GloBenin"			},
    	    { "61701", "CELLPLUS-MRU"		},
    	    { "61710", "EMTEL"			},
    	    { "61801", "LoneStar"			},
    	    { "61802", "LIBERCELL"			},
    	    { "61807", "Celcom"			},
    	    { "61901", "CELTEL SL"			},
    	    { "61902", "MILLICOM"			},
    	    { "62001", "GH MTN"			},
    	    { "62002", "ONEtouch"			},
    	    { "62003", "tiGO"			},
    	    { "62006", "GH Zain"			},
    	    { "62100", "MTN"				},
    	    { "62120", "Zain NG"			},
    	    { "62130", "MTN-NG"			},
    	    { "62140", "NG Mtel"			},
    	    { "62150", "Glo NG"			},
    	    { "62160", "EMTS NGA"			},
    	    { "62201", "CELTEL TCD"			},
    	    { "62302", "Telecel"			},
    	    { "62304", "NationLink"			},
    	    { "62401", "MTN CAM"			},
    	    { "62402", "Orange CAM"			},
    	    { "62501", "CPV MOVEL"			},
    	    { "62502", "CPV T+"			},
    	    { "62601", "CSTmovel"			},
    	    { "62701", "GETESA"			},
    	    { "62801", "LIBERTIS"			},
    	    { "62802", "TELECEL"			},
    	    { "62803", "ZAIN GA"			},
    	    { "62901", "CELTEL"			},
    	    { "62907", "WARID RC"			},
    	    { "62910", "MTN-CG"			},
    	    { "63001", "VODACOM CD"			},
    	    { "63002", "CELTEL DRC"			},
    	    { "63004", "CELLCO"			},
    	    { "63005", "SCELL"			},
    	    { "63086", "CCT"				},
    	    { "63089", "OASIS"			},
    	    { "63102", "UNITEL"			},
    	    { "63202", "MTN"				},
    	    { "63203", "Orange Bissau"		},
    	    { "63207", "GTM"				},
    	    { "63301", "C&W SEY"			},
    	    { "63310", "AIRTEL"			},
    	    { "63401", "MobiTel SDN"			},
    	    { "63402", "MTN"				},
    	    { "63405", "Vivacell"			},
    	    { "63510", "R-CELL"			},
    	    { "63512", "RWTEL"			},
    	    { "63601", "ET-MTN"			},
    	    { "63701", "SO Telesom"			},
    	    { "63704", "SOMAFONE"			},
    	    { "63730", "Golis"			},
    	    { "63782", "Telsom"			},
    	    { "63801", "EVATIS"			},
    	    { "63902", "Safaricom"			},
    	    { "63903", "CELTEL"			},
    	    { "63907", "GSM Telkom"			},
    	    { "64001", "TRITEL"			},
    	    { "64002", "MOBITEL"			},
    	    { "64003", "ZANTEL"			},
    	    { "64004", "VodaCom"			},
    	    { "64005", "celtel"			},
    	    { "64009", "Hits TZ"			},
    	    { "64101", "UG CelTel"			},
    	    { "64110", "MTN-UGANDA"			},
    	    { "64111", "Uganda Telecom"		},
    	    { "64114", "OUL"				},
    	    { "64122", "WaridTel"			},
    	    { "64201", "EWB"				},
    	    { "64202", "TEMPO-AF"			},
    	    { "64203", "ONATEL"			},
    	    { "64282", "TELECEL-BDI"			},
    	    { "64301", "mCel"			},
    	    { "64304", "VodaCom-MZ"			},
    	    { "64501", "ZM CELTEL"			},
    	    { "64502", "MTN ZM"			},
    	    { "64601", "ZAIN MG"			},
    	    { "64602", "Orange MG"			},
    	    { "64604", "TELMA"			},
    	    { "64700", "Orange re"			},
    	    { "64702", "OUTREMER"			},
    	    { "64710", "SFR REUNION"			},
    	    { "64801", "ZW NET*ONE"			},
    	    { "64803", "TELECEL ZW"			},
    	    { "64804", "ZW ECONET"			},
    	    { "64901", "MTC NAMIBIA"			},
    	    { "64903", "NAM Cell One"		},
    	    { "65001", "TNM"				},
    	    { "65010", "CELTEL MW"			},
    	    { "65101", "Vodacom-LS"			},
    	    { "65102", "EZI-CEL"			},
    	    { "65201", "MASCOM"			},
    	    { "65202", "Orange"			},
    	    { "65310", "Swazi-MTN"			},
    	    { "65401", "HURI"			},
    	    { "65501", "VodaCom-SA"			},
    	    { "65507", "Cell C"			},
    	    { "65510", "MTN-SA"			},
    	    { "70267", "BTL"				},
    	    { "70401", "ClaroGTM"			},
    	    { "70402", "COMCEL"			},
    	    { "70403", "movistar"			},
    	    { "70601", "ClaroSLV"			},
    	    { "70602", "DIGICEL"			},
    	    { "70603", "TELEMOVIL"			},
    	    { "70604", "movistar"			},
    	    { "70610", "PERSONAL"			},
    	    { "70801", "Claro HND"			},
    	    { "70802", "CELTEL"			},
    	    { "70830", "HT - 200"			},
    	    { "71021", "ClaroNIC"			},
    	    { "71030", "MOVISTARNI"			},
    	    { "71073", "ClaroNIC"			},
    	    { "71201", "I.C.E."			},
    	    { "71202", "I.C.E."			},
    	    { "71401", "PANCW"			},
    	    { "71403", "CLARO PA"			},
    	    { "71420", "movistar"			},
    	    { "71606", "MOVISTAR"			},
    	    { "71610", "Claro PER"			},
    	    { "72207", "movistar"			},
    	    { "72231", "CLARO AR"			},
    	    { "72234", "AR TP"			},
    	    { "72235", "PORT-HABLE"			},
    	    { "72402", "TIM"				},
    	    { "72403", "TIM"				},
    	    { "72404", "TIM"				},
    	    { "72405", "Claro"			},
    	    { "72406", "VIVO"			},
    	    { "72410", "VIVO"			},
    	    { "72411", "VIVO"			},
    	    { "72415", "BRA SCTL"			},
    	    { "72416", "BRA BrTCelular"		},
    	    { "72423", "VIVO"			},
    	    { "72424", "AMAZONIA"			},
    	    { "72431", "Oi"				},
    	    { "72432", "CTBC"			},
    	    { "72433", "CTBC"			},
    	    { "72434", "CTBC"			},
    	    { "73001", "ENTEL PCS"			},
    	    { "73002", "movistar"			},
    	    { "73003", "ClaroCHL"			},
    	    { "73010", "ENTEL PCS"			},
    	    { "73401", "DIGITEL"			},
    	    { "73402", "DIGITEL"			},
    	    { "73403", "DIGITEL"			},
    	    { "73404", "movistar"			},
    	    { "73406", "MOVILNET"			},
    	    { "73601", "VIVA"			},
    	    { "73602", "BOMOV"			},
    	    { "73603", "Telecel"			},
    	    { "73801", "DIGICEL"			},
    	    { "73802", "CLNK PLS"			},
    	    { "74000", "movistar"			},
    	    { "74001", "PORTAGSM"			},
    	    { "74401", "HPGYSA"			},
    	    { "74402", "CLARO PY"			},
    	    { "74404", "Telecel"			},
    	    { "74405", "Personal"			},
    	    { "74601", "ICMS"			},
    	    { "74602", "TeleG"			},
    	    { "74603", "DIGICEL"			},
    	    { "74604", "UNIQA"			},
    	    { "74807", "movistar"			},
    	    { "74810", "CLARO UY"			},
    	    { "75001", "C&W FLK"			},
    	    { "79502", "TM CELL"			},
    	    { "90105", "Thuraya"			},
    	    { "90106", "Thuraya"			},
    	    { "90112", "MCP"				},
    	    { "90114", "AeroMobile"			},
    	    { "90115", "OnAir"			},
    	    { "90117", "Navitas"			},
    	    { "90118", "WMS"	      			},
    	    { "90121", "Seanet"			},
    	    { "310020", "T-Mobile"		},
    	    { "310032", "IT&E"		      	},
    	    { "310070", "AT&T"			},
    	    { "310090", "Edge Wireless"	     	},
    	    { "310140", "GTA Wireless "	      	},
    	    { "310300", "iSmartUS"			},
    	    { "310370", "Guam Cellular & Paging Inc" 	},
    	    { "310995", "Google Android"	      	},
    	    { "310996", "Google Android"		},
    	    { "311001", "Wilkes USA"		      	},
    	    { "311005", "Wilkes USA"		      	},
    	    { "311240", "USACWCI"		      	},
    	    { "311360", "Stelera Wireless"		},
    	    { "311370", "GCI"				},
    	    { "311530", "USANCW"			},
    	    { "334050", "Iusacell"                    },
    	    { "348170", "C&W"				},
    	    { "362951", "CHIPPIE"		      	},
    	    { "376352", "IslandCom TCI"		},
    	    { "405025", "TATA DOCOMO"			},
    	    { "405027", "TATA DOCOMO"			},
    	    { "405029", "TATA DOCOMO"			},
    	    { "405030", "TATA DOCOMO"			},
    	    { "405031", "TATA DOCOMO"			},
    	    { "405032", "TATA DOCOMO"			},
    	    { "405034", "TATA DOCOMO"			},
    	    { "405035", "TATA DOCOMO"			},
    	    { "405036", "TATA DOCOMO"			},
    	    { "405037", "TATA DOCOMO"			},
    	    { "405038", "TATA DOCOMO"			},
    	    { "405039", "TATA DOCOMO"			},
    	    { "405041", "TATA DOCOMO"			},
    	    { "405042", "TATA DOCOMO"			},
    	    { "405043", "TATA DOCOMO"			},
    	    { "405044", "TATA DOCOMO"			},
    	    { "405045", "TATA DOCOMO"			},
    	    { "405046", "TATA DOCOMO"			},
    	    { "405047", "TATA DOCOMO"			},
    	    { "405750", "Vodafone IN"			},
    	    { "405751", "Vodafone IN"			},
    	    { "405752", "Vodafone IN"			},
    	    { "405753", "Vodafone IN"			},
    	    { "405754", "Vodafone IN"			},
    	    { "405755", "Vodafone IN"			},
    	    { "405756", "Vodafone IN"			},
    	    { "405799", "!DEA"			},
    	    { "405800", "Aircel"			},
    	    { "405801", "Aircel"			},
    	    { "405802", "Aircel"			},
    	    { "405803", "Aircel"			},
    	    { "405804", "Aircel"			},
    	    { "405805", "Aircel"			},
    	    { "405806", "Aircel"			},
    	    { "405807", "Aircel"			},
    	    { "405808", "Aircel"			},
    	    { "405809", "Aircel"			},
    	    { "405810", "Aircel"			},
    	    { "405811", "Aircel"			},
    	    { "405812", "Aircel"			},
    	    { "405813", "uninor"			},
    	    { "405814", "uninor"			},
    	    { "405815", "uninor"			},
    	    { "405816", "uninor"			},
    	    { "405817", "uninor"			},
    	    { "405818", "uninor"			},
    	    { "405819", "uninor"			},
    	    { "405820", "uninor"			},
    	    { "405821", "uninor"			},
    	    { "405822", "uninor"			},
    	    { "405844", "uninor"			},
    	    { "405845", "!DEA"			},
    	    { "405846", "!DEA"			},
    	    { "405848", "!DEA"			},
    	    { "405849", "!DEA"			},
    	    { "405850", "!DEA"			},
    	    { "405852", "!DEA"			},
    	    { "405853", "!DEA"			},
    	    { "405854", "Loop"			},
    	    { "405855", "Loop"			},
    	    { "405856", "Loop"			},
    	    { "405857", "Loop"			},
    	    { "405858", "Loop"			},
    	    { "405859", "Loop"			},
    	    { "405860", "Loop"			},
    	    { "405861", "Loop"			},
    	    { "405862", "Loop"			},
    	    { "405863", "Loop"			},
    	    { "405864", "Loop"			},
    	    { "405865", "Loop"			},
    	    { "405866", "Loop"			},
    	    { "405867", "Loop"			},
    	    { "405868", "Loop"			},
    	    { "405869", "Loop"			},
    	    { "405870", "Loop"			},
    	    { "405871", "Loop"			},
    	    { "405872", "Loop"			},
    	    { "405873", "Loop"			},
    	    { "405874", "Loop"			},
    	    { "405875", "uninor"			},
    	    { "405876", "uninor"			},
    	    { "405877", "uninor"			},
    	    { "405878", "uninor"			},
    	    { "405879", "uninor"			},
    	    { "405880", "uninor"			},
    	    { "405925", "uninor"			},
    	    { "405926", "uninor"			},
    	    { "405927", "uninor"			},
    	    { "405928", "uninor"			},
    	    { "405929", "uninor"			},
    	    { "706010", "PERSONAL"			},
    	    { "708001", "Claro HND"			},
    	    { "710300", "movistar"			},
    	    { "732101", "COMCEL"			},
    	    { "732111", "OLA"				},
    	    { "732123", "COL Movistar"		},
    };
    // -[bug0001494 android_team jackyyang 20120314]
    
    private void setSignalStrengthDefaultValues() {
        mSignalStrength = new SignalStrength(99, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, true);
    }

    /* [Bug0001532 Nusmart alex. for quicker register.] */
    private void queueNextNetworkPoll() {
        if (cm.getRadioState().isCdma()) {
            return;
        }

        Message msg;

        msg = obtainMessage();
        msg.what = EVENT_NETWORK_STATE_CHANGED;

        if(DBG)
           log("@@@Nusmart--queueNextNetworkPoll---send Message.@@@");

        sendMessageDelayed(msg, POLL_N_PERIOD_MILLIS);
    }
    /* [Bug0001532 Nusmart alex. for quicker register.] */

    /**
     * A complete "service state" from our perspective is
     * composed of a handful of separate requests to the radio.
     *
     * We make all of these requests at once, but then abandon them
     * and start over again if the radio notifies us that some
     * event has changed
     */
    private void pollState() {

        switch (cm.getRadioState()) {
            case RADIO_UNAVAILABLE:
                newSS.setStateOutOfService();
                newCellLoc.setStateInvalid();
                setSignalStrengthDefaultValues();
                mGotCountryCode = false;
                pollStateDone();
            break;

            case RADIO_OFF:
                newSS.setStateOff();
                newCellLoc.setStateInvalid();
                setSignalStrengthDefaultValues();
                mGotCountryCode = false;
                pollStateDone();
            break;

            case RUIM_NOT_READY:
            case RUIM_READY:
            case RUIM_LOCKED_OR_ABSENT:
            case NV_NOT_READY:
            case NV_READY:
                if (DBG) log("Radio Technology Change ongoing, setting SS to off");
                newSS.setStateOff();
                newCellLoc.setStateInvalid();
                setSignalStrengthDefaultValues();
                mGotCountryCode = false;

                //NOTE: pollStateDone() is not needed in this case
                break;

            default:
                /* [Bug0001532 Nusmart alex. for quicker register.] */
                if(phone.getContext().getResources().getBoolean(
                        com.android.internal.R.bool.config_radioQuickRegister)){
                   if(pollNetworkState && pollingContext != null){
                      log("@@@Nusmart code for faster register.@@@");
                      queueNextNetworkPoll();
                      return;
                   }
                }
                /* [Bug0001532 Nusmart alex. for quicker register.] */

                pollingContext = new int[1];
                pollingContext[0] = 0;

                // Issue all poll-related commands at once
                // then count down the responses, which
                // are allowed to arrive out-of-order

                pollingContext[0]++;
                cm.getOperator(
                    obtainMessage(
                        EVENT_POLL_STATE_OPERATOR, pollingContext));

                pollingContext[0]++;
                cm.getDataRegistrationState(
                    obtainMessage(
                        EVENT_POLL_STATE_GPRS, pollingContext));

                pollingContext[0]++;
                cm.getVoiceRegistrationState(
                    obtainMessage(
                        EVENT_POLL_STATE_REGISTRATION, pollingContext));

                pollingContext[0]++;
                cm.getNetworkSelectionMode(
                    obtainMessage(
                        EVENT_POLL_STATE_NETWORK_SELECTION_MODE, pollingContext));

                /** [Bug0001532 Nusmart alex. for quicker register.] */
                if(phone.getContext().getResources().getBoolean(
                        com.android.internal.R.bool.config_radioQuickRegister)){
                   if(pollNetworkState_f)
                   {
                      pollNetworkState = true;
                      pollNetworkState_f = false;
                   }
                }
                /** [Bug0001532 Nusmart alex. for quicker register.] */
            break;
        }
    }

    private void pollStateDone() {
        if (DBG) {
            log("Poll ServiceState done: " +
                " oldSS=[" + ss + "] newSS=[" + newSS +
                "] oldGprs=" + gprsState + " newData=" + newGPRSState +
                " oldMaxDataCalls=" + mMaxDataCalls +
                " mNewMaxDataCalls=" + mNewMaxDataCalls +
                " oldReasonDataDenied=" + mReasonDataDenied +
                " mNewReasonDataDenied=" + mNewReasonDataDenied +
                " oldType=" + ServiceState.radioTechnologyToString(mRadioTechnology) +
                " newType=" + ServiceState.radioTechnologyToString(mNewRadioTechnology));
        }

        boolean hasRegistered =
            ss.getState() != ServiceState.STATE_IN_SERVICE
            && newSS.getState() == ServiceState.STATE_IN_SERVICE;

        boolean hasDeregistered =
            ss.getState() == ServiceState.STATE_IN_SERVICE
            && newSS.getState() != ServiceState.STATE_IN_SERVICE;

        boolean hasGprsAttached =
                gprsState != ServiceState.STATE_IN_SERVICE
                && newGPRSState == ServiceState.STATE_IN_SERVICE;

        boolean hasGprsDetached =
                gprsState == ServiceState.STATE_IN_SERVICE
                && newGPRSState != ServiceState.STATE_IN_SERVICE;

        boolean hasRadioTechnologyChanged = mRadioTechnology != mNewRadioTechnology;

        boolean hasChanged = !newSS.equals(ss);

        boolean hasRoamingOn = !ss.getRoaming() && newSS.getRoaming();

        boolean hasRoamingOff = ss.getRoaming() && !newSS.getRoaming();

        boolean hasLocationChanged = !newCellLoc.equals(cellLoc);

        // Add an event log when connection state changes
        if (ss.getState() != newSS.getState() || gprsState != newGPRSState) {
            EventLog.writeEvent(EventLogTags.GSM_SERVICE_STATE_CHANGE,
                ss.getState(), gprsState, newSS.getState(), newGPRSState);
        }

        ServiceState tss;
        tss = ss;
        ss = newSS;
        newSS = tss;
        // clean slate for next time
        newSS.setStateOutOfService();

        GsmCellLocation tcl = cellLoc;
        cellLoc = newCellLoc;
        newCellLoc = tcl;

        // Add an event log when network type switched
        // TODO: we may add filtering to reduce the event logged,
        // i.e. check preferred network setting, only switch to 2G, etc
        if (hasRadioTechnologyChanged) {
            int cid = -1;
            GsmCellLocation loc = ((GsmCellLocation)phone.getCellLocation());
            if (loc != null) cid = loc.getCid();
            EventLog.writeEvent(EventLogTags.GSM_RAT_SWITCHED, cid, mRadioTechnology,
                    mNewRadioTechnology);
            if (DBG) {
                log("RAT switched " + ServiceState.radioTechnologyToString(mRadioTechnology) +
                        " -> " + ServiceState.radioTechnologyToString(mNewRadioTechnology) +
                        " at cell " + cid);
            }
        }

        gprsState = newGPRSState;
        mReasonDataDenied = mNewReasonDataDenied;
        mMaxDataCalls = mNewMaxDataCalls;
        mRadioTechnology = mNewRadioTechnology;
        // this new state has been applied - forget it until we get a new new state
        mNewRadioTechnology = 0;


        newSS.setStateOutOfService(); // clean slate for next time

        if (hasRadioTechnologyChanged) {
            phone.setSystemProperty(TelephonyProperties.PROPERTY_DATA_NETWORK_TYPE,
                    ServiceState.radioTechnologyToString(mRadioTechnology));
        }

        if (hasRegistered) {
            mNetworkAttachedRegistrants.notifyRegistrants();
        }

        if (hasChanged) {
            String operatorNumeric;

            updateSpnDisplay();

            phone.setSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_ALPHA,
                ss.getOperatorAlphaLong());

            operatorNumeric = ss.getOperatorNumeric();
            phone.setSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_NUMERIC, operatorNumeric);

            if (operatorNumeric == null) {
                phone.setSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_ISO_COUNTRY, "");
                mGotCountryCode = false;
            } else {
                String iso = "";
                try{
                    iso = MccTable.countryCodeForMcc(Integer.parseInt(
                            operatorNumeric.substring(0,3)));
                } catch ( NumberFormatException ex){
                    loge("countryCodeForMcc error" + ex);
                } catch ( StringIndexOutOfBoundsException ex) {
                    loge("countryCodeForMcc error" + ex);
                }

                phone.setSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_ISO_COUNTRY, iso);
                mGotCountryCode = true;

                if (mNeedFixZone) {
                    TimeZone zone = null;
                    // If the offset is (0, false) and the timezone property
                    // is set, use the timezone property rather than
                    // GMT.
                    String zoneName = SystemProperties.get(TIMEZONE_PROPERTY);
                    if ((mZoneOffset == 0) && (mZoneDst == false) &&
                        (zoneName != null) && (zoneName.length() > 0) &&
                        (Arrays.binarySearch(GMT_COUNTRY_CODES, iso) < 0)) {
                        zone = TimeZone.getDefault();
                        // For NITZ string without timezone,
                        // need adjust time to reflect default timezone setting
                        long tzOffset;
                        tzOffset = zone.getOffset(System.currentTimeMillis());
                        if (getAutoTime()) {
                            setAndBroadcastNetworkSetTime(System.currentTimeMillis() - tzOffset);
                        } else {
                            // Adjust the saved NITZ time to account for tzOffset.
                            mSavedTime = mSavedTime - tzOffset;
                        }
                    } else if (iso.equals("")){
                        // Country code not found.  This is likely a test network.
                        // Get a TimeZone based only on the NITZ parameters (best guess).
                        zone = getNitzTimeZone(mZoneOffset, mZoneDst, mZoneTime);
                    } else {
                        zone = TimeUtils.getTimeZone(mZoneOffset,
                            mZoneDst, mZoneTime, iso);
                    }

                    mNeedFixZone = false;

                    if (zone != null) {
                        if (getAutoTimeZone()) {
                            setAndBroadcastNetworkSetTimeZone(zone.getID());
                        }
                        saveNitzTimeZone(zone.getID());
                    }
                }
            }

            phone.setSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_ISROAMING,
                ss.getRoaming() ? "true" : "false");

            phone.notifyServiceStateChanged(ss);
        }

        if (hasGprsAttached) {
            mAttachedRegistrants.notifyRegistrants();
        }

        if (hasGprsDetached) {
            mDetachedRegistrants.notifyRegistrants();
        }

        if (hasRadioTechnologyChanged) {
            phone.notifyDataConnection(Phone.REASON_NW_TYPE_CHANGED);
        }

        if (hasRoamingOn) {
            mRoamingOnRegistrants.notifyRegistrants();
        }

        if (hasRoamingOff) {
            mRoamingOffRegistrants.notifyRegistrants();
        }

        if (hasLocationChanged) {
            phone.notifyLocationChanged();
        }

        if (! isGprsConsistent(gprsState, ss.getState())) {
            if (!mStartedGprsRegCheck && !mReportedGprsNoReg) {
                mStartedGprsRegCheck = true;

                int check_period = Settings.Secure.getInt(
                        phone.getContext().getContentResolver(),
                        Settings.Secure.GPRS_REGISTER_CHECK_PERIOD_MS,
                        DEFAULT_GPRS_CHECK_PERIOD_MILLIS);
                sendMessageDelayed(obtainMessage(EVENT_CHECK_REPORT_GPRS),
                        check_period);
            }
        } else {
            mReportedGprsNoReg = false;
        }
    }

    /**
     * Check if GPRS got registered while voice is registered.
     *
     * @param gprsState for GPRS registration state, i.e. CGREG in GSM
     * @param serviceState for voice registration state, i.e. CREG in GSM
     * @return false if device only register to voice but not gprs
     */
    private boolean isGprsConsistent(int gprsState, int serviceState) {
        return !((serviceState == ServiceState.STATE_IN_SERVICE) &&
                (gprsState != ServiceState.STATE_IN_SERVICE));
    }

    /**
     * Returns a TimeZone object based only on parameters from the NITZ string.
     */
    private TimeZone getNitzTimeZone(int offset, boolean dst, long when) {
        TimeZone guess = findTimeZone(offset, dst, when);
        if (guess == null) {
            // Couldn't find a proper timezone.  Perhaps the DST data is wrong.
            guess = findTimeZone(offset, !dst, when);
        }
        if (DBG) log("getNitzTimeZone returning " + (guess == null ? guess : guess.getID()));
        return guess;
    }

    private TimeZone findTimeZone(int offset, boolean dst, long when) {
        int rawOffset = offset;
        if (dst) {
            rawOffset -= 3600000;
        }
        String[] zones = TimeZone.getAvailableIDs(rawOffset);
        TimeZone guess = null;
        Date d = new Date(when);
        for (String zone : zones) {
            TimeZone tz = TimeZone.getTimeZone(zone);
            if (tz.getOffset(when) == offset &&
                tz.inDaylightTime(d) == dst) {
                guess = tz;
                break;
            }
        }

        return guess;
    }

    private void queueNextSignalStrengthPoll() {
        if (dontPollSignalStrength || (cm.getRadioState().isCdma())) {
            // The radio is telling us about signal strength changes
            // we don't have to ask it
            return;
        }

        Message msg;

        msg = obtainMessage();
        msg.what = EVENT_POLL_SIGNAL_STRENGTH;

        long nextTime;

        // TODO Don't poll signal strength if screen is off
        sendMessageDelayed(msg, POLL_PERIOD_MILLIS);
    }

    /**
     *  Send signal-strength-changed notification if changed.
     *  Called both for solicited and unsolicited signal strength updates.
     */
    private void onSignalStrengthResult(AsyncResult ar) {
        SignalStrength oldSignalStrength = mSignalStrength;
        int rssi = 99;
        int lteSignalStrength = -1;
        int lteRsrp = -1;
        int lteRsrq = -1;
        int lteRssnr = -1;
        int lteCqi = -1;

        if (ar.exception != null) {
            // -1 = unknown
            // most likely radio is resetting/disconnected
            setSignalStrengthDefaultValues();
        } else {
            int[] ints = (int[])ar.result;

            // bug 658816 seems to be a case where the result is 0-length
            if (ints.length != 0) {
                rssi = ints[0];
                lteSignalStrength = ints[7];
                lteRsrp = ints[8];
                lteRsrq = ints[9];
                lteRssnr = ints[10];
                lteCqi = ints[11];
            } else {
                loge("Bogus signal strength response");
                rssi = 99;
            }
        }

        mSignalStrength = new SignalStrength(rssi, -1, -1, -1,
                -1, -1, -1, lteSignalStrength, lteRsrp, lteRsrq, lteRssnr, lteCqi, true);

        if (!mSignalStrength.equals(oldSignalStrength)) {
            try { // This takes care of delayed EVENT_POLL_SIGNAL_STRENGTH (scheduled after
                  // POLL_PERIOD_MILLIS) during Radio Technology Change)
                phone.notifySignalStrength();
           } catch (NullPointerException ex) {
                log("onSignalStrengthResult() Phone already destroyed: " + ex
                        + "SignalStrength not notified");
           }
        }
    }

    /**
     * Set restricted state based on the OnRestrictedStateChanged notification
     * If any voice or packet restricted state changes, trigger a UI
     * notification and notify registrants when sim is ready.
     *
     * @param ar an int value of RIL_RESTRICTED_STATE_*
     */
    private void onRestrictedStateChanged(AsyncResult ar) {
        RestrictedState newRs = new RestrictedState();

        if (DBG) log("onRestrictedStateChanged: E rs "+ mRestrictedState);

        if (ar.exception == null) {
            int[] ints = (int[])ar.result;
            int state = ints[0];

            newRs.setCsEmergencyRestricted(
                    ((state & RILConstants.RIL_RESTRICTED_STATE_CS_EMERGENCY) != 0) ||
                    ((state & RILConstants.RIL_RESTRICTED_STATE_CS_ALL) != 0) );
            //ignore the normal call and data restricted state before SIM READY
            if (phone.getIccCard().getState() == IccCard.State.READY) {
                newRs.setCsNormalRestricted(
                        ((state & RILConstants.RIL_RESTRICTED_STATE_CS_NORMAL) != 0) ||
                        ((state & RILConstants.RIL_RESTRICTED_STATE_CS_ALL) != 0) );
                newRs.setPsRestricted(
                        (state & RILConstants.RIL_RESTRICTED_STATE_PS_ALL)!= 0);
            }

            if (DBG) log("onRestrictedStateChanged: new rs "+ newRs);

            if (!mRestrictedState.isPsRestricted() && newRs.isPsRestricted()) {
                mPsRestrictEnabledRegistrants.notifyRegistrants();
                setNotification(PS_ENABLED);
            } else if (mRestrictedState.isPsRestricted() && !newRs.isPsRestricted()) {
                mPsRestrictDisabledRegistrants.notifyRegistrants();
                setNotification(PS_DISABLED);
            }

            /**
             * There are two kind of cs restriction, normal and emergency. So
             * there are 4 x 4 combinations in current and new restricted states
             * and we only need to notify when state is changed.
             */
            if (mRestrictedState.isCsRestricted()) {
                if (!newRs.isCsRestricted()) {
                    // remove all restriction
                    setNotification(CS_DISABLED);
                } else if (!newRs.isCsNormalRestricted()) {
                    // remove normal restriction
                    setNotification(CS_EMERGENCY_ENABLED);
                } else if (!newRs.isCsEmergencyRestricted()) {
                    // remove emergency restriction
                    setNotification(CS_NORMAL_ENABLED);
                }
            } else if (mRestrictedState.isCsEmergencyRestricted() &&
                    !mRestrictedState.isCsNormalRestricted()) {
                if (!newRs.isCsRestricted()) {
                    // remove all restriction
                    setNotification(CS_DISABLED);
                } else if (newRs.isCsRestricted()) {
                    // enable all restriction
                    setNotification(CS_ENABLED);
                } else if (newRs.isCsNormalRestricted()) {
                    // remove emergency restriction and enable normal restriction
                    setNotification(CS_NORMAL_ENABLED);
                }
            } else if (!mRestrictedState.isCsEmergencyRestricted() &&
                    mRestrictedState.isCsNormalRestricted()) {
                if (!newRs.isCsRestricted()) {
                    // remove all restriction
                    setNotification(CS_DISABLED);
                } else if (newRs.isCsRestricted()) {
                    // enable all restriction
                    setNotification(CS_ENABLED);
                } else if (newRs.isCsEmergencyRestricted()) {
                    // remove normal restriction and enable emergency restriction
                    setNotification(CS_EMERGENCY_ENABLED);
                }
            } else {
                if (newRs.isCsRestricted()) {
                    // enable all restriction
                    setNotification(CS_ENABLED);
                } else if (newRs.isCsEmergencyRestricted()) {
                    // enable emergency restriction
                    setNotification(CS_EMERGENCY_ENABLED);
                } else if (newRs.isCsNormalRestricted()) {
                    // enable normal restriction
                    setNotification(CS_NORMAL_ENABLED);
                }
            }

            mRestrictedState = newRs;
        }
        log("onRestrictedStateChanged: X rs "+ mRestrictedState);
    }

    /** code is registration state 0-5 from TS 27.007 7.2 */
    private int regCodeToServiceState(int code) {
        switch (code) {
            case 0:
            case 2: // 2 is "searching"
            case 3: // 3 is "registration denied"
            case 4: // 4 is "unknown" no vaild in current baseband
            case 10:// same as 0, but indicates that emergency call is possible.
            case 12:// same as 2, but indicates that emergency call is possible.
            case 13:// same as 3, but indicates that emergency call is possible.
            case 14:// same as 4, but indicates that emergency call is possible.
                return ServiceState.STATE_OUT_OF_SERVICE;

            case 1:
                return ServiceState.STATE_IN_SERVICE;

            case 5:
                // in service, roam
                return ServiceState.STATE_IN_SERVICE;

            default:
                loge("regCodeToServiceState: unexpected service state " + code);
                return ServiceState.STATE_OUT_OF_SERVICE;
        }
    }


    /**
     * code is registration state 0-5 from TS 27.007 7.2
     * returns true if registered roam, false otherwise
     */
    private boolean regCodeIsRoaming (int code) {
        // 5 is  "in service -- roam"
        return 5 == code;
    }

    /**
     * Set roaming state when gsmRoaming is true and, if operator mcc is the
     * same as sim mcc, ons is different from spn
     * @param gsmRoaming TS 27.007 7.2 CREG registered roaming
     * @param s ServiceState hold current ons
     * @return true for roaming state set
     */
    private boolean isRoamingBetweenOperators(boolean gsmRoaming, ServiceState s) {
        String spn = SystemProperties.get(TelephonyProperties.PROPERTY_ICC_OPERATOR_ALPHA, "empty");

        String onsl = s.getOperatorAlphaLong();
        String onss = s.getOperatorAlphaShort();

        boolean equalsOnsl = onsl != null && spn.equals(onsl);
        boolean equalsOnss = onss != null && spn.equals(onss);

        String simNumeric = SystemProperties.get(
                TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, "");
        String  operatorNumeric = s.getOperatorNumeric();

        boolean equalsMcc = true;
        try {
            equalsMcc = simNumeric.substring(0, 3).
                    equals(operatorNumeric.substring(0, 3));
        } catch (Exception e){
        }

        return gsmRoaming && !(equalsMcc && (equalsOnsl || equalsOnss));
    }

    private static int twoDigitsAt(String s, int offset) {
        int a, b;

        a = Character.digit(s.charAt(offset), 10);
        b = Character.digit(s.charAt(offset+1), 10);

        if (a < 0 || b < 0) {

            throw new RuntimeException("invalid format");
        }

        return a*10 + b;
    }

    /**
     * @return The current GPRS state. IN_SERVICE is the same as "attached"
     * and OUT_OF_SERVICE is the same as detached.
     */
    int getCurrentGprsState() {
        return gprsState;
    }

    public int getCurrentDataConnectionState() {
        return gprsState;
    }

    /**
     * @return true if phone is camping on a technology (eg UMTS)
     * that could support voice and data simultaneously.
     */
    public boolean isConcurrentVoiceAndDataAllowed() {
        return (mRadioTechnology >= ServiceState.RADIO_TECHNOLOGY_UMTS);
    }

    /**
     * Provides the name of the algorithmic time zone for the specified
     * offset.  Taken from TimeZone.java.
     */
    private static String displayNameFor(int off) {
        off = off / 1000 / 60;

        char[] buf = new char[9];
        buf[0] = 'G';
        buf[1] = 'M';
        buf[2] = 'T';

        if (off < 0) {
            buf[3] = '-';
            off = -off;
        } else {
            buf[3] = '+';
        }

        int hours = off / 60;
        int minutes = off % 60;

        buf[4] = (char) ('0' + hours / 10);
        buf[5] = (char) ('0' + hours % 10);

        buf[6] = ':';

        buf[7] = (char) ('0' + minutes / 10);
        buf[8] = (char) ('0' + minutes % 10);

        return new String(buf);
    }

    /**
     * nitzReceiveTime is time_t that the NITZ time was posted
     */
    private void setTimeFromNITZString (String nitz, long nitzReceiveTime) {
        // "yy/mm/dd,hh:mm:ss(+/-)tz"
        // tz is in number of quarter-hours

        long start = SystemClock.elapsedRealtime();
        if (DBG) {log("NITZ: " + nitz + "," + nitzReceiveTime +
                        " start=" + start + " delay=" + (start - nitzReceiveTime));
        }

        try {
            /* NITZ time (hour:min:sec) will be in UTC but it supplies the timezone
             * offset as well (which we won't worry about until later) */
            Calendar c = Calendar.getInstance(TimeZone.getTimeZone("GMT"));

            c.clear();
            c.set(Calendar.DST_OFFSET, 0);

            String[] nitzSubs = nitz.split("[/:,+-]");

            int year = 2000 + Integer.parseInt(nitzSubs[0]);
            c.set(Calendar.YEAR, year);

            // month is 0 based!
            int month = Integer.parseInt(nitzSubs[1]) - 1;
            c.set(Calendar.MONTH, month);

            int date = Integer.parseInt(nitzSubs[2]);
            c.set(Calendar.DATE, date);

            int hour = Integer.parseInt(nitzSubs[3]);
            c.set(Calendar.HOUR, hour);

            int minute = Integer.parseInt(nitzSubs[4]);
            c.set(Calendar.MINUTE, minute);

            int second = Integer.parseInt(nitzSubs[5]);
            c.set(Calendar.SECOND, second);

            boolean sign = (nitz.indexOf('-') == -1);

            int tzOffset = Integer.parseInt(nitzSubs[6]);

            int dst = (nitzSubs.length >= 8 ) ? Integer.parseInt(nitzSubs[7])
                                              : 0;

            // The zone offset received from NITZ is for current local time,
            // so DST correction is already applied.  Don't add it again.
            //
            // tzOffset += dst * 4;
            //
            // We could unapply it if we wanted the raw offset.

            tzOffset = (sign ? 1 : -1) * tzOffset * 15 * 60 * 1000;

            TimeZone    zone = null;

            // As a special extension, the Android emulator appends the name of
            // the host computer's timezone to the nitz string. this is zoneinfo
            // timezone name of the form Area!Location or Area!Location!SubLocation
            // so we need to convert the ! into /
            if (nitzSubs.length >= 9) {
                String  tzname = nitzSubs[8].replace('!','/');
                zone = TimeZone.getTimeZone( tzname );
            }

            String iso = SystemProperties.get(TelephonyProperties.PROPERTY_OPERATOR_ISO_COUNTRY);

            if (zone == null) {

                if (mGotCountryCode) {
                    if (iso != null && iso.length() > 0) {
                        zone = TimeUtils.getTimeZone(tzOffset, dst != 0,
                                c.getTimeInMillis(),
                                iso);
                    } else {
                        // We don't have a valid iso country code.  This is
                        // most likely because we're on a test network that's
                        // using a bogus MCC (eg, "001"), so get a TimeZone
                        // based only on the NITZ parameters.
                        zone = getNitzTimeZone(tzOffset, (dst != 0), c.getTimeInMillis());
                    }
                }
            }

            if (zone == null) {
                // We got the time before the country, so we don't know
                // how to identify the DST rules yet.  Save the information
                // and hope to fix it up later.

                mNeedFixZone = true;
                mZoneOffset  = tzOffset;
                mZoneDst     = dst != 0;
                mZoneTime    = c.getTimeInMillis();
            }

            if (zone != null) {
                if (getAutoTimeZone()) {
                    setAndBroadcastNetworkSetTimeZone(zone.getID());
                }
                saveNitzTimeZone(zone.getID());
            }

            String ignore = SystemProperties.get("gsm.ignore-nitz");
            if (ignore != null && ignore.equals("yes")) {
                log("NITZ: Not setting clock because gsm.ignore-nitz is set");
                return;
            }

            try {
                mWakeLock.acquire();

                if (getAutoTime()) {
                    long millisSinceNitzReceived
                            = SystemClock.elapsedRealtime() - nitzReceiveTime;

                    if (millisSinceNitzReceived < 0) {
                        // Sanity check: something is wrong
                        if (DBG) {
                            log("NITZ: not setting time, clock has rolled "
                                            + "backwards since NITZ time was received, "
                                            + nitz);
                        }
                        return;
                    }

                    if (millisSinceNitzReceived > Integer.MAX_VALUE) {
                        // If the time is this far off, something is wrong > 24 days!
                        if (DBG) {
                            log("NITZ: not setting time, processing has taken "
                                        + (millisSinceNitzReceived / (1000 * 60 * 60 * 24))
                                        + " days");
                        }
                        return;
                    }

                    // Note: with range checks above, cast to int is safe
                    c.add(Calendar.MILLISECOND, (int)millisSinceNitzReceived);

                    if (DBG) {
                        log("NITZ: Setting time of day to " + c.getTime()
                            + " NITZ receive delay(ms): " + millisSinceNitzReceived
                            + " gained(ms): "
                            + (c.getTimeInMillis() - System.currentTimeMillis())
                            + " from " + nitz);
                    }

                    setAndBroadcastNetworkSetTime(c.getTimeInMillis());
                    Log.i(LOG_TAG, "NITZ: after Setting time of day");
                }
                SystemProperties.set("gsm.nitz.time", String.valueOf(c.getTimeInMillis()));
                saveNitzTime(c.getTimeInMillis());
                if (false) {
                    long end = SystemClock.elapsedRealtime();
                    log("NITZ: end=" + end + " dur=" + (end - start));
                }
            } finally {
                mWakeLock.release();
            }
        } catch (RuntimeException ex) {
            loge("NITZ: Parsing NITZ time " + nitz + " ex=" + ex);
        }
    }

    private boolean getAutoTime() {
        try {
            return Settings.System.getInt(phone.getContext().getContentResolver(),
                    Settings.System.AUTO_TIME) > 0;
        } catch (SettingNotFoundException snfe) {
            return true;
        }
    }

    private boolean getAutoTimeZone() {
        try {
            return Settings.System.getInt(phone.getContext().getContentResolver(),
                    Settings.System.AUTO_TIME_ZONE) > 0;
        } catch (SettingNotFoundException snfe) {
            return true;
        }
    }

    private void saveNitzTimeZone(String zoneId) {
        mSavedTimeZone = zoneId;
    }

    private void saveNitzTime(long time) {
        mSavedTime = time;
        mSavedAtTime = SystemClock.elapsedRealtime();
    }

    /**
     * Set the timezone and send out a sticky broadcast so the system can
     * determine if the timezone was set by the carrier.
     *
     * @param zoneId timezone set by carrier
     */
    private void setAndBroadcastNetworkSetTimeZone(String zoneId) {
        AlarmManager alarm =
            (AlarmManager) phone.getContext().getSystemService(Context.ALARM_SERVICE);
        alarm.setTimeZone(zoneId);
        Intent intent = new Intent(TelephonyIntents.ACTION_NETWORK_SET_TIMEZONE);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra("time-zone", zoneId);
        phone.getContext().sendStickyBroadcast(intent);
    }

    /**
     * Set the time and Send out a sticky broadcast so the system can determine
     * if the time was set by the carrier.
     *
     * @param time time set by network
     */
    private void setAndBroadcastNetworkSetTime(long time) {
        SystemClock.setCurrentTimeMillis(time);
        Intent intent = new Intent(TelephonyIntents.ACTION_NETWORK_SET_TIME);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra("time", time);
        phone.getContext().sendStickyBroadcast(intent);
    }

    private void revertToNitzTime() {
        if (Settings.System.getInt(phone.getContext().getContentResolver(),
                Settings.System.AUTO_TIME, 0) == 0) {
            return;
        }
        if (DBG) {
            log("Reverting to NITZ Time: mSavedTime=" + mSavedTime
                + " mSavedAtTime=" + mSavedAtTime);
        }
        if (mSavedTime != 0 && mSavedAtTime != 0) {
            setAndBroadcastNetworkSetTime(mSavedTime
                    + (SystemClock.elapsedRealtime() - mSavedAtTime));
        }
    }

    private void revertToNitzTimeZone() {
        if (Settings.System.getInt(phone.getContext().getContentResolver(),
                Settings.System.AUTO_TIME_ZONE, 0) == 0) {
            return;
        }
        if (DBG) log("Reverting to NITZ TimeZone: tz='" + mSavedTimeZone);
        if (mSavedTimeZone != null) {
            setAndBroadcastNetworkSetTimeZone(mSavedTimeZone);
        }
    }

    /**
     * Post a notification to NotificationManager for restricted state
     *
     * @param notifyType is one state of PS/CS_*_ENABLE/DISABLE
     */
    private void setNotification(int notifyType) {

        if (DBG) log("setNotification: create notification " + notifyType);
        Context context = phone.getContext();

        mNotification = new Notification();
        mNotification.when = System.currentTimeMillis();
        mNotification.flags = Notification.FLAG_AUTO_CANCEL;
        mNotification.icon = com.android.internal.R.drawable.stat_sys_warning;
        Intent intent = new Intent();
        mNotification.contentIntent = PendingIntent
        .getActivity(context, 0, intent, PendingIntent.FLAG_CANCEL_CURRENT);

        CharSequence details = "";
        CharSequence title = context.getText(com.android.internal.R.string.RestrictedChangedTitle);
        int notificationId = CS_NOTIFICATION;

        switch (notifyType) {
        case PS_ENABLED:
            notificationId = PS_NOTIFICATION;
            details = context.getText(com.android.internal.R.string.RestrictedOnData);;
            break;
        case PS_DISABLED:
            notificationId = PS_NOTIFICATION;
            break;
        case CS_ENABLED:
            details = context.getText(com.android.internal.R.string.RestrictedOnAllVoice);;
            break;
        case CS_NORMAL_ENABLED:
            details = context.getText(com.android.internal.R.string.RestrictedOnNormal);;
            break;
        case CS_EMERGENCY_ENABLED:
            details = context.getText(com.android.internal.R.string.RestrictedOnEmergency);;
            break;
        case CS_DISABLED:
            // do nothing and cancel the notification later
            break;
        }

        if (DBG) log("setNotification: put notification " + title + " / " +details);
        mNotification.tickerText = title;
        mNotification.setLatestEventInfo(context, title, details,
                mNotification.contentIntent);

        NotificationManager notificationManager = (NotificationManager)
            context.getSystemService(Context.NOTIFICATION_SERVICE);

        if (notifyType == PS_DISABLED || notifyType == CS_DISABLED) {
            // cancel previous post notification
            notificationManager.cancel(notificationId);
        } else {
            // update restricted state notification
            notificationManager.notify(notificationId, mNotification);
        }
    }

    @Override
    protected void log(String s) {
        Log.d(LOG_TAG, "[GsmSST] " + s);
    }

    @Override
    protected void loge(String s) {
        Log.e(LOG_TAG, "[GsmSST] " + s);
    }

    private static void sloge(String s) {
        Log.e(LOG_TAG, "[GsmSST] " + s);
    }
}
