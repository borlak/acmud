CC      = gcc
C_FLAGS = -I/usr/include/mysql
C_FLAGS += -Wall -Wextra -g3 -ggdb3 $(PROF) $(NOCRYPT)
L_FLAGS = -L/usr/local/lib -lpthread -L/usr/lib64/mysql -lmysqlclient
DATE = `date +%m%d%y`

O_FILES = \
	area.c \
	cmdio.c \
	command.c \
	const.c \
	convert.c \
	creature.c \
	editor.c \
	flags.c \
	info.c \
	io.c \
	main.c \
	newdel.c \
	object.c \
	os.c \
	search.c \
	socket.c \
	update.c \
	util.c \
	wizard.c

acm: $(O_FILES)
	rm -f acm
	$(CC) -o acm $(O_FILES) $(L_FLAGS)
  
# make love, not distclean
love: $(O_FILES)
	@echo "<3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3"
	rm -f acm
	$(CC) -o acm $(O_FILES) $(L_FLAGS)
	@echo "<3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3"
  
.c.o:
	$(CC) -c $(C_FLAGS) $<
  
clean:
	rm -rf *.o
	touch *.[ch]
	make
  
tar:
	rm -rf *.o
	tar czhf ../acm$(DATE).tgz *.*

