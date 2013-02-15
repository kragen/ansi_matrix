#include "sound.h"

#ifdef __linux__

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void u8_audio_write(unsigned char *samples, int n_samples) {
  static FILE *output;
  if (!output) {
    output = popen("aplay", "w");
    if (!output) {
      perror("popen");
      abort();
    }
  }
  //printf("writing %d samples %02x%02x%02x%02x\r\n", n_samples, samples[0], samples[1], samples[2], samples[3]);
  fwrite(samples, n_samples, 1, output);
  fflush(output);
}

#else

#error u8_audio_write not defined yet for this platform; can you write it?

#endif
