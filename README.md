# SO-projeto-2

Server:

./bin/server info_files 3 /tmp/pipe_servidor

          Executable/level_dir/max_games/server_pipe_path
gdb --args bin/server info_files 1 /tmp/pipe_servidor



Client:

./bin/client 1 /tmp/pipe_servidor info_client_files/1.p

           Executable/id/server_pipe_path/pacman_file
gdb --args bin/client 1 /tmp/pipe_servidor info_client_files/1.p



Thread Sanitizer stuff:

sudo sysctl vm.mmap_rnd_bits=28

bin/server info_files 1 /tmp/pipe_servidor 2> tsan_output.log ????
bin/client 10 /tmp/pipe_servidor client/info_files/1.p2> tsan_output.log ????