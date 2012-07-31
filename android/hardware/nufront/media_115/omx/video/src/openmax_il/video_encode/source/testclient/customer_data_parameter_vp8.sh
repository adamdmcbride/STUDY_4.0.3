#!/bin/bash
#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Hantro Products Oy.                             --
#-                                                                            --
#-                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--  Abstract : Test script
#--
#-------------------------------------------------------------------------------

RGB_SEQUENCE_HOME=${YUV_SEQUENCE_HOME}/rgb

# Input YUV files
in18=${YUV_SEQUENCE_HOME}/qcif/metro2_25fps_qcif.yuv
in22=${YUV_SEQUENCE_HOME}/synthetic/kuuba_random_w352h288.uyvy422
in84=${YUV_SEQUENCE_HOME}/synthetic/rangetest_w352h288.yuv

# Intra release case
in100=${YUV_SEQUENCE_HOME}/cif/metro_25fps_cif.yuv

# RGB input
in270=${RGB_SEQUENCE_HOME}/1080p/rush_hour_25fps_w1920h1080.rgb565
in271=${RGB_SEQUENCE_HOME}/vga/shields_60fps_vga.rgb888


# Dummy
in999=/dev/zero

# H.264 stream name, do not change
out=stream.ivf

# Default parameters
colorConversion=( 0 0 0 0 0 )

case "$1" in
3000|3001|3002|3003|3004) {
	valid=(			    0	    0	    0	    0	    0	    )
	input=(			    $in100	$in18	$in270	$in84	$in22   )
	output=(		    $out	$out	$out	$out	$out	)
	firstVop=(		    0	    0	    0	    0	    0	    )
	lastVop=(		    100	    30	    5	    30	    1	    )
	lumWidthSrc=(	    352	    176	    1920	352	    352	    )
	lumHeightSrc=(	    288	    144	    1080	288	    288	    )
	width=(			    352	    176	    1280	144	    144	    )
	height=(		    288	    144	    1024	96	    96	    )
	horOffsetSrc=(		0	    0	    19	    120	    3	    )
	verOffsetSrc=(		0	    0	    17	    96	    1	    )
	outputRateNumer=(	25	    30	    30	    30	    30	    )
	outputRateDenom=(	1	    1	    1	    1	    1	    )
	inputRateNumer=(	25	    30	    30	    30	    30	    )
	inputRateDenom=(	1	    1	    1	    1	    1	    )
	intraVopRate=(		1	    0	    1	    0	    0	    )
	dctPartitions=(		0	    0	    0	    0	    0	    )
	errorResilient=(	0	    0	    0	    0	    0	    )
	ipolFilter=(    	0	    0	    0	    0	    0	    )
	filterType=(    	0	    0	    0	    0	    0	    )
	bitPerSecond=(		384000	512000	0	    0	    0	    )
	vopRc=(			    1	    1	    0	    0	    0	    )
	vopSkip=(		    0	    0	    0	    0	    0	    )
	qpHdr=(			    51	    25	    35	    35	    10	    )
	qpMin=(			    0	    0	    0	    0	    0	    )
	qpMax=(			    51	    51	    51	    51	    51	    )
	level=(			    1	    1	    1	    1	    1	    )
	mbRc=(			    0	    0	    0	    0	    0	    )
	inputFormat=(		0	    0	    4	    0	    3	    )
	rotation=(		    0	    0	    1	    0	    0	    )
	stabilization=(		0	    0	    0	    1	    0	    )
	testId=(		    0	    0	    0	    0	    0	    )
        category=("VP8_intra"
                  "VP8_rate_control"
                  "pre_processing"
                  "VP8_stabilization"
                  "VP8_cropping")
	desc=(	"VP8 Intra release test. Size 352x288, begins with QP 51. VopRc, 384 Kbps."
		"Test rate control. Normal operation with Qcif @ 512 Kbps."
		"Test RGB input. Max Frame size SXGA, rot. 0, crop 19x17."
		"Test stabilization. Frame size 144x96. IF 0, rot. 0."
		"Test cropping. Offset 3x1. Frame size 144x96. IF 3, rot. 0.")
	};;

    3005) {
	valid=(			    0	)
	input=(			    $in271	)
	output=(		    $out	)
	firstVop=(		    0   )
	lastVop=(		    30	)
	lumWidthSrc=(	    640	)
	lumHeightSrc=(	    480	)
	width=(			    640	)
	height=(		    480	)
	horOffsetSrc=(		0   )
	verOffsetSrc=(		0   )
	outputRateNumer=(	30	)
	outputRateDenom=(	1	)
	inputRateNumer=(	30	)
	inputRateDenom=(	1	)
	intraVopRate=(		0	)
	dctPartitions=(		0	)
	errorResilient=(	0	)
	ipolFilter=(    	0	)
	filterType=(    	0	)
	bitPerSecond=(		1024000	)
	vopRc=(			    1	)
	vopSkip=(		    0	)
	qpHdr=(			    35	)
	qpMin=(			    0	)
	qpMax=(			    51	)
	level=(			    1	)
	mbRc=(			    0	)
	inputFormat=(		10	)
	rotation=(		    0	)
	stabilization=(		0	)
	testId=(		    0	)
        category=( "pre_processing")
	desc=(	"Test 32 bit RGB input. Frame size VGA, rot. 0.")
	};;

	* )
	valid=(			-1	-1	-1	-1	-1	);;
esac
