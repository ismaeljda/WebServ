NAME = WebServ
SRC = Request/RequestParser.cpp src/main.cpp Request/utils.cpp Request/HttpStatusCodes.cpp Server_2/Server.cpp Config/ConfigParser.cpp
OBJS = ${SRC:.cpp=.o}
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98


all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)


%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re