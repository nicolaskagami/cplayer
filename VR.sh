#!/bin/bash
destination="127.0.0.1"
video_id="video_tiled"
tile_height="8"
tile_width="8"
viewport_height="4"
viewport_width="4"
priority_height="2"
priority_width="2"
segment_count="70"

valgrind ./cplayer_vr $destination $video_id $tile_height $tile_width $viewport_height $viewport_width $priority_height $priority_width $segment_count `uuid`
