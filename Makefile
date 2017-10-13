all: cplayer cplayer_vr

cplayer: cplayer.c
	gcc cplayer.c -lcurl -o cplayer
cplayer_vr: cplayer_vr.c
	gcc cplayer_vr.c -lcurl -o cplayer_vr
