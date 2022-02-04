# GNU Make 3.81; MacOSX gcc 4.2.1; clang 19.6.0; MacOSX MinGW 4.3.0

# https://stackoverflow.com/questions/18136918/how-to-get-current-relative-directory-of-your-makefile
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

project := $(current_dir)

# dirs
src    := src
test   := test
build  := build
bin    := bin
backup := backup
doc    := doc
media  := media
#lemon  := lemon
PREFIX := /usr/local

# files in $(bin)
install := $(project)-`date +%Y-%m-%d`

# extra stuff we should back up
extra :=

# John Graham-Cumming: rwildcard is a recursive wildcard
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) \
$(filter $(subst *,%,$2),$d))

java_srcs    := $(call rwildcard, $(src), *.java)
all_c_srcs   := $(call rwildcard, $(src), *.c)
c_re_srcs    := $(call rwildcard, $(src), *.re.c)
c_rec_srcs   := $(call rwildcard, $(src), *.re_c.c)
c_gperf_srcs := $(call rwildcard, $(src), *.gperf.c)
c_srcs       := $(filter-out $(c_re_srcs) $(c_rec_srcs) $(c_gperf_srcs), $(all_c_srcs))
h_srcs       := $(call rwildcard, $(src), *.h)
y_srcs       := $(call rwildcard, $(src), *.y)
all_c_tests  := $(call rwildcard, $(test), *.c)
c_re_tests   := $(call rwildcard, $(test), *.re.c)
c_rec_tests  := $(call rwildcard, $(test), *.re_c.c)
c_tests      := $(filter-out $(c_re_tests) $(c_rec_tests), $(all_c_tests))
h_tests      := $(call rwildcard, $(test), *.h)
icons        := $(call rwildcard, $(media), *.ico)

# combinations
all_h      := $(h_srcs) $(h_tests)
all_srcs   := $(java_srcs) $(all_c_srcs) $(y_srcs)
all_tests  := $(all_c_tests)
all_icons  := $(icons)

java_class := $(patsubst $(src)/%.java, $(build)/%.class, $(java_srcs))
c_objs     := $(patsubst $(src)/%.c, $(build)/%.o, $(c_srcs))
# must not conflict, eg, foo.c.re and foo.c would go to the same thing
c_re_builds := $(patsubst $(src)/%.re.c, $(build)/%.c, $(c_re_srcs))
c_re_test_builds := $(patsubst $(test)/%.re.c, $(build)/$(test)/%.c, $(c_re_tests))
c_rec_builds := $(patsubst $(src)/%.re_c.c, $(build)/%.c, $(c_rec_srcs))
c_rec_test_builds := $(patsubst $(test)/%.re_c.c, $(build)/%.c, $(c_rec_tests))
c_y_builds := $(patsubst $(src)/%.y, $(build)/%.c, $(y_srcs))
c_gperf_builds := $(patsubst $(src)/%.gperf.c, $(build)/%.c, $(c_gperf_srcs))
# together .re/.re_c/.y/.gperf.c
c_other_objs := $(patsubst $(build)/%.c, $(build)/%.o, $(c_re_builds) \
$(c_rec_builds) $(c_re_test_builds) $(c_rec_test_builds) $(c_y_builds) $(c_gperf_builds))
test_c_objs := $(patsubst $(test)/%.c, $(build)/$(test)/%.o, $(c_tests))
html_docs  := $(patsubst $(src)/%.c, $(doc)/%.html, $(c_srcs))

cdoc  := cdoc
re2c  := re2c
mkdir := mkdir -p
cat   := cat
zip   := zip
bison := bison
#lemon := lemon
gperf := gperf

target    := # -mwindows
optimize  := -ffast-math
warnbasic := -Wall -pedantic -ansi # -std=c99
# Some stuff is really new.
warnclang := -Wextra \
-Weverything \
-Wno-comma \
-Wno-logical-op-parentheses \
-Wno-parentheses \
-Wno-poison-system-directories \
-Wno-documentation-unknown-command \
-Wno-documentation \
-Wno-shift-op-parentheses \
-Wno-empty-body \
-Wno-padded \
-Wno-switch-enum \
-Wno-missing-noreturn
warn := $(warnbasic) $(warnclang)

CC   := clang # gcc
CF   := $(target) $(optimize) $(warn)
OF   := # -lm -framework OpenGL -framework GLUT or -lglut -lGLEW

# Jakob Borg and Eldar Abusalimov
# $(ARGS) is all the extra arguments; $(BRGS) is_all_the_extra_arguments
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
ifeq (backup, $(firstword $(MAKECMDGOALS)))
  ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  BRGS := $(subst $(SPACE),_,$(ARGS))
  ifneq (,$(BRGS))
    BRGS := -$(BRGS)
  endif
  $(eval $(ARGS):;@:)
endif
ifeq (release, $(firstword $(MAKECMDGOALS)))
	CF += -funroll-loops -Ofast -D NDEBUG # -O3
	OF += -Ofast
else
	CF += -g
endif

######
# compiles the programme by default

default: $(bin)/$(project)
	# . . . success; executable is in $(bin)/$(project)

docs: $(html_docs)

# linking
$(bin)/$(project): $(c_objs) $(c_other_objs) $(test_c_objs)
	# linking rule
	@$(mkdir) $(bin)
	$(CC) $(OF) -o $@ $^

# compiling
#$(lemon)/$(bin)/$(lem): $(lemon)/$(src)/lemon.c
#	# compiling lemon
#	@$(mkdir) $(lemon)/$(bin)
#	$(CC) $(CF) -o $@ $<

$(c_objs): $(build)/%.o: $(src)/%.c $(all_h)
	# c_objs rule
	@$(mkdir) $(build)
	$(CC) $(CF) -c -o $@ $<

$(c_other_objs): $(build)/%.o: $(build)/%.c $(all_h)
	# c_other_objs rule
	$(CC) $(CF) -c -o $@ $<

$(test_c_objs): $(build)/$(test)/%.o: $(test)/%.c $(all_h)
	# test_c_objs rule
	@$(mkdir) $(build)
	@$(mkdir) $(build)/$(test)
	$(CC) $(CF) -c -o $@ $<

# -8 made my file 32767 lines or longer

$(c_re_builds): $(build)/%.c: $(src)/%.re.c
	# *.re.c build rule
	@$(mkdir) $(build)
	$(re2c) -W -T -o $@ $<

$(c_re_test_builds): $(build)/$(test)/%.c: $(test)/%.re.c
	# *.re.c tests rule
	@$(mkdir) $(build)
	@$(mkdir) $(build)/$(test)
	$(re2c) -W -T -o $@ $<

$(c_rec_builds): $(build)/%.c: $(src)/%.re_c.c
	# *.re_c.c (conditions) build rule
	@$(mkdir) $(build)
	$(re2c) -W -T -c -o $@ $<

$(c_rec_test_builds): $(build)/$(test)/%.c: $(test)/%.re_c.c
	# *.re_c.c (conditions) tests rule
	@$(mkdir) $(build)
	@$(mkdir) $(build)/$(test)
	$(re2c) -W -T -c -o $@ $<

$(c_y_builds): $(build)/%.c: $(src)/%.y # $(lemon)/$(bin)/$(lem)
	# .y rule
	@$(mkdir) $(build)
	$(bison) -o $@ $<

$(c_gperf_builds): $(build)/%.c: $(src)/%.gperf.c
	# *.gperf.c build rule
	@$(mkdir) $(build)
	$(gperf) $@ --output-file $<

$(html_docs): $(doc)/%.html: $(src)/%.c $(src)/%.h
	# docs rule
	@$(mkdir) $(doc)
	cat $^ | $(cdoc) > $@

######
# phoney targets

.PHONY: setup clean backup icon install uninstall test docs release

clean:
	-rm -f $(c_objs) $(test_c_objs) $(c_other_objs) $(c_re_builds) \
$(c_rec_builds) $(html_docs)
	-rm -rf $(bin)/$(test)

backup:
	@$(mkdir) $(backup)
	$(zip) $(backup)/$(project)-`date +%Y-%m-%dT%H%M%S`$(BRGS).zip \
readme.txt Makefile $(all_h) $(all_srcs) $(all_tests) $(all_icons)

icon: default
	# . . . setting icon on a Mac.
	cp $(media)/$(icon) $(bin)/$(icon)
	-sips --addIcon $(bin)/$(icon)
	-DeRez -only icns $(bin)/$(icon) > $(bin)/$(RSRC)
	-Rez -append $(bin)/$(RSRC) -o $(bin)/$(project)
	-SetFile -a C $(bin)/$(project)

setup: default icon
	@$(mkdir) $(bin)/$(install)
	cp $(bin)/$(project) readme.txt $(bin)/$(install)
	rm -f $(bin)/$(install)-MacOSX.dmg
	# or rm -f $(BDIR)/$(INST)-Win32.zip
	hdiutil create $(bin)/$(install)-MacOSX.dmg -volname "$(project)" -srcfolder $(bin)/$(install)
	# or zip $(BDIR)/$(INST)-Win32.zip -r $(BDIR)/$(INST)
	rm -R $(bin)/$(install)

# this needs work
release: clean default
	strip $(bin)/$(project)
	# define NDEBUG

install: release
	@$(mkdir) -p $(DESTDIR)$(PREFIX)/bin
	cp $(bin)/$(project) $(DESTDIR)$(PREFIX)/bin/$(project)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(project)

docs: $(html_docs)
