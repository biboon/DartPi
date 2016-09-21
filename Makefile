# Makefile général
# Constantes pour la compilation des programmes
export APP = main
export CC = gcc
export AR = ar
export CFLAGS = -Wall


# Dossiers du projet
DIRS=Threads App

# La cible generale
all: $(patsubst %, _dir_%, $(DIRS))

$(patsubst %,_dir_%,$(DIRS)):
	cd $(patsubst _dir_%,%,$@) && make

# La cible de nettoyage
clean: $(patsubst %, _clean_%, $(DIRS))

$(patsubst %,_clean_%,$(DIRS)):
	cd $(patsubst _clean_%,%,$@) && make clean

# Compilation en mode debug
debug: CFLAGS += -DDEBUG -g
debug: $(patsubst %, _dir_%, $(DIRS))
