all:
	$(CC) $(LDFLAGS) -Wall -ggdb -lpthread -llo -D_REENTRANT -o bin/noya src/config.c src/thread_input.c src/thread_manager.c src/thread_renderer.c src/main.c `pkg-config --cflags --libs clutter-0.8`

clean:
	@rm bin/noya
