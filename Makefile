# Nom de l'exécutable
EXEC = max_grid

# Fichier source
SRC = max_in_grid_opt.c

# Compilateurs disponibles
CC_GCC = gcc
CC_CLANG = clang

# Options de compilation
CFLAGS = -O2 -g -march=native -funroll-loops -ffast-math -Wall -fno-omit-frame-pointer -grecord-gcc-switches
# Paramètres par défaut pour l'exécution
REPS = 3
SIZE_X = 2000
SIZE_Y = 3000

# Compilation avec GCC
gcc: $(SRC)
	$(CC_GCC) $(CFLAGS) $(SRC) -o $(EXEC)_gcc

# Compilation avec Clang
clang: $(SRC)
	$(CC_CLANG) $(CFLAGS) $(SRC) -o $(EXEC)_clang

# Exécuter le programme avec GCC
run_gcc: gcc
	./$(EXEC)_gcc $(REPS) $(SIZE_X) $(SIZE_Y)

# Exécuter le programme avec Clang
run_clang: clang
	./$(EXEC)_clang $(REPS) $(SIZE_X) $(SIZE_Y)

# Tester l'exécution avec les deux compilateurs et comparer les temps d'exécution
test: gcc clang
	@echo "Testing execution with GCC:"
	@time ./$(EXEC)_gcc $(REPS) $(SIZE_X) $(SIZE_Y)
	@echo "\nTesting execution with Clang:"
	@time ./$(EXEC)_clang $(REPS) $(SIZE_X) $(SIZE_Y)

# Nettoyage des fichiers binaires
clean:
	rm -f $(EXEC)_gcc $(EXEC)_clang

