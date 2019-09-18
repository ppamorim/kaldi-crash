#include "online2-pcm-nnet3-decode-faster.h"

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char *argv[]) {

  signal(SIGSEGV, handler);   // install our handler

  if (argc < 15) {
    std::cout << "Your command has only " << argc << " arguments, please insert the last one (file path)." << std::endl;
    return 0;
  }

  std::cout << "Number of settings:" << argc << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::cout << argv[i] << std::endl;
  }

  char * file_path = argv[14];
  std::cout << "Processing audio from file path: " << file_path << std::endl;

  kaldi::PcmDecoder* pcmDecoder = new kaldi::PcmDecoder(argc-1, argv);

  FILE * fh = fopen(file_path, "r");
  int nread;
  size_t chunk_len = pcmDecoder->chunk_len;
  std::cout << "chunk_len: " << chunk_len << std::endl;

  int16 * buff = new int16[chunk_len];

  while((nread = fread(buff, sizeof(int16), chunk_len, fh)) > 0) {
    //CODE REMOVED
    //Here the data is sent to kaldi and processed.
  }

  std::cout << "Completed" << std::endl;
  delete pcmDecoder;

  return 0;
}
