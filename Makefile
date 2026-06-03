# Makefile général de la série FEM.
# Construit chaque outil dans son sous-dossier (chacun a son propre Makefile).
# Les briques numériques communes vivent dans core/ (en-têtes, rien à compiler).

TOOLS := babyfem kidfem

.PHONY: all $(TOOLS) examples clean

all: $(TOOLS)

# Construit un outil : `make babyfem` ou `make kidfem`
$(TOOLS):
	@$(MAKE) --no-print-directory -C $@

# Exemples C++ historiques de babyfem (optionnel)
examples:
	@$(MAKE) --no-print-directory -C babyfem/examples

clean:
	@$(MAKE) --no-print-directory -C babyfem clean
	@$(MAKE) --no-print-directory -C kidfem  clean
	@$(MAKE) --no-print-directory -C babyfem/examples clean
