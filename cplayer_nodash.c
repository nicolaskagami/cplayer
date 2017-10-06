/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * Very simple HTTP GET
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <errno.h>
#define BUFFER_LIMIT 60
#define INTERVAL 10
#define SEGMENT_TIME 10
#define AVERAGE_WINDOW_SIZE 3

double tvdiff_secs(struct timeval newer, struct timeval older)
{
  if(newer.tv_sec != older.tv_sec)
    return (double)(newer.tv_sec-older.tv_sec)+
      (double)(newer.tv_usec-older.tv_usec)/1000000.0;
  return (double)(newer.tv_usec-older.tv_usec)/1000000.0;
}


int main(int argc, char **argv)
{
  CURL *curl;
  CURLcode res;
  int stall_count = 0;
  double *avg_window = (double *)malloc(AVERAGE_WINDOW_SIZE * sizeof(double));
  int j;
  for (j = 0; j < AVERAGE_WINDOW_SIZE; j++){
     avg_window[j] = 0.0;
  }
  int current_index;
  int segment_count_i;
  struct timeval beg, end, last_load, former_last_load;
  double elapsed, startup_time, buffer_time, time_from_last_load, stall_len, bitrate;
  int i = 1; 
  if (argc < 5){
     printf("usage: cplayer <destination> <video_id> <segment_count> <uuid>\n:");
     return(-1);
  }
  double volume;
  double ttime;
  char *destination = argv[1];
  char *video_id = argv[2];
  char *segment_count = argv[3];
  char *uuid = argv[4];
  segment_count_i = atoi(segment_count);
  char url[200];
  char resolution[5];
  resolution[0] = '\0';
  strcat(resolution, "1080");
  strcat(resolution, "\0");
  char filename[80];
  filename[0] = '\0';
  strcat(filename, "videologs/");
  strcat(filename, uuid);
  strcat(filename, ".log");
  strcat(filename, "\0");
  FILE *flog = fopen(filename, "wb");
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    FILE *f = fopen("/dev/null", "wb");
    gettimeofday(&beg, NULL);
    while (i <= segment_count_i){
        current_index = (i - 1) % AVERAGE_WINDOW_SIZE;
        url[0] = '\0';
        strcat(url, "http://");
	strcat(url, destination);
	strcat(url, "/");
	strcat(url, video_id);
        strcat(url, "-");
        strcat(url, resolution);
	strcat(url, "/");
	strcat(url, video_id);
	strcat(url, "-seg");
        char index[8];
        sprintf(index, "%d", i);
	strcat(url, index);
	strcat(url, ".mp4");
	strcat(url, "\0");
	//printf("url: %s SC: %d\n", url, segment_count_i);
 
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
        //curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
     /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        gettimeofday(&last_load, NULL);
        elapsed = tvdiff_secs(last_load, beg);

        curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &volume);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ttime);

        bitrate = (8.0 * volume) / ttime;
        //printf("LAST BITRATE: %.06lf\n", bitrate);
        avg_window[current_index] = bitrate;
        int local_avg_window_size = i < AVERAGE_WINDOW_SIZE ? i : AVERAGE_WINDOW_SIZE;
        double local_sum = 0.0; 
        for (j = 0; j < local_avg_window_size; j++){
           //printf("BR [%d] = %.06lf\n", j, avg_window[j]);
           local_sum += avg_window[j];
        }
        double local_avg = local_sum / local_avg_window_size;
        //printf("local_avg: %.06lf\n", local_avg);
        
        double ref = (local_avg < bitrate ? local_avg : bitrate);
        //printf("REF: %.06lf\n", ref);
        //printf("%lf %lf %lf\n", volume, ttime, bitrate);
        fprintf(flog, "%lf;%d;%s;%lf;%lf;%lf\n", elapsed, i, resolution, ref, local_avg, bitrate);
        fflush(flog);


        if (ref <= 1200000){
            resolution[0] = '\0';
            strcat(resolution, "1080");
            strcat(resolution, "\0");
        }else if (ref <= 2100000){
            resolution[0] = '\0';
            strcat(resolution, "1080");
            strcat(resolution, "\0");
        }else{
            resolution[0] = '\0';
            strcat(resolution, "1080");
            strcat(resolution, "\0");
        }

        //printf("DOWN ELAPSED: %.6lf\n", tvdiff_secs(last_load, inspect));
        if (i == 1){
           startup_time = elapsed;
           buffer_time = SEGMENT_TIME;
        }else{
           time_from_last_load = tvdiff_secs(last_load, former_last_load);
           buffer_time = buffer_time - time_from_last_load;
           if (buffer_time < 0){
              stall_count++;
              stall_len = stall_len - buffer_time;
              buffer_time = SEGMENT_TIME;
           }else{
              buffer_time = buffer_time + SEGMENT_TIME;
           }
        }
        former_last_load = last_load;
        //printf("BT: %lf, %d\n", buffer_time, BUFFER_LIMIT);
        if (buffer_time > BUFFER_LIMIT){
           //printf("SLEEPANDO\n");
           sleep(INTERVAL);
        }
        i++;
    }
    gettimeofday(&end, NULL);
    fclose(f);
    fclose(flog);
    free(avg_window);
    printf("%lu.%06lu;%.6lf;%.6lf;%.6lf;%d\n", end.tv_sec, end.tv_usec, tvdiff_secs(end, beg), startup_time, stall_len, stall_count);
    /* always cleanup */
    curl_easy_cleanup(curl);
  }else{
      printf("ERROR: %s\n", strerror(errno));
      return(-1);
  }
  return 0;
}
