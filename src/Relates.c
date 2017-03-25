/** 2017 Neil Edelman, distributed under the terms of the MIT License;
 see readme.txt, or \url{ https://opensource.org/licenses/MIT }.

 {Relates} is a wrapper for the recursive {Relate}, which stores an association
 between key-value {Text} fields, and children {Relate}s in a tree.

 @title		Relates
 @author	Neil
 @std		C89/90
 @version	1.0; 2017-02
 @since		1.0; 2017-02 */

#include <stdlib.h> /* malloc free */
#include <stdio.h>  /* fprintf */
#include <errno.h>	/* errno */
#include <string.h>	/* strerror */
#include <limits.h>	/* INT_MAX */
#include <ctype.h>	/* isspace */
#include "Relates.h"



/* for allocating recursively */
static const unsigned fibonacci6  = 8;
static const unsigned fibonacci7  = 13;

/* for allocating value */
static const unsigned fibonacci11 = 89;
static const unsigned fibonacci12 = 144;

enum Error {
	E_NO_ERROR,
	E_ERRNO,
	E_PARAMETER,
	E_OVERFLOW,
	E_SYNTAX,
	E_ASSERT,
	E_PARENT
};

static const char *const error_explination[] = {
	"no error",
	0,
	"parameter out-of-range",
	"overflow",
	"syntax error",
	"assertion failed",
	"null parent"
};

/* globals */

static enum Error global_error = E_NO_ERROR;
static int        global_errno_copy;

struct Relates {
	struct Relate *root;
	enum Error error;
	int errno_copy;
};

struct Relate {
	/* parent pointer -- used for error reporting, mostly */
	struct Parent {
		enum { T_RELATE, T_ROOT } type;
		union { struct Relate *relate; struct Relates *root; } p;
	} parent;
	/* node */
	struct Text *key, *value;
	struct RelateParent *key_parent, *value_parent;
	/* recursive */
	size_t size, capacity[2]; struct Relate **childs;
};

/* private */

static int relate(struct Relate *this, const struct Parent parent);
static void relate_(struct Relate **const this_ptr);
static struct Relate *relate_new(struct Relate *this);
static int relate_grow(struct Relate *const this);
static struct Relates *to_relates(struct Relate *const this);

/**********
 * Public */

/** Constructs a generic, {Text} with {root_name} as it's root name, or blank
 if null.
 @throws E_PARAMETER, E_ERRNO */
struct Relates *Relates(void) {
	struct Relates *this;
	struct Parent parent;

	if(!(this = malloc(sizeof *this + sizeof *this->root)))
		{ global_error = E_ERRNO, global_errno_copy = errno; return 0; }
	this->root       = (struct Relate *)(this + 1);
	this->error      = E_NO_ERROR;
	this->errno_copy = 0;
	parent.type      = T_ROOT;
	parent.p.root    = this;
	/* initialise root to root_name if specified */
	if(!relate(this->root, parent)) {
		global_error = this->error, global_errno_copy = this->errno_copy;
		Relates_(&this);
		return 0;
	}

	return this;
}

/** Destructs a {Text}. */
void Relates_(struct Relates **const this_ptr) {
	struct Relates *this;

	if(!this_ptr || !(this = *this_ptr)) return;
	relate_(&this->root);
	free(this);
	*this_ptr = 0;
}

/** @return The {Relate} root of the {Relates}. */
struct Relate *RelatesGetRoot(struct Relates *const this) {
	return this ? this->root : 0;
}

/** @return The last error associated with {this} (can be null.) */
const char *RelatesGetError(struct Relates *const this) {
	const char *str;
	enum Error *perr;
	int *perrno;

	perr   = this ? &this->error      : &global_error;
	perrno = this ? &this->errno_copy : &global_errno_copy;
	if(!(str = error_explination[*perr])) str = strerror(*perrno);
	*perr = 0;
	return str;
}

/**********
 * Relate */

/** Get key string; shortcut for {TextGet(RelateGetKey(this))}. */
const char *RelateKey(const struct Relate *const this) {
	return this ? TextGet(this->key) : 0;
}

/** Get value string; shortcut for {TextGet(RelateGetValue(this))}. */
const char *RelateValue(const struct Relate *const this) {
	return this ? TextGet(this->value) : 0;
}

/** Get key. */
struct Text *RelateGetKey(const struct Relate *const this) {
	return this ? this->key : 0;
}

/** Get value. */
struct Text *RelateGetValue(const struct Relate *const this) {
	return this ? this->value : 0;
}

/** Get {RelateParent}, a read-only structure that describes how the key is
 derived from the parent. */
const struct RelateParent *RelateGetKeyParent(const struct Relate *const this) {
	return this ? this->key_parent : 0;
}

/** Get {RelateParent}, a read-only structure that describes how the value is
 derived from the parent. */
const struct RelateParent *RelateGetValueParent(const struct Relate *const
	this) {
	return this ? this->value_parent : 0;
}

/** New child. */
struct Relate *RelateNewChild(struct Relate *const this) {
	return this ? relate_new(this) : 0;
}

/** E {r} \in {this.children}, {p}({r}) -> {a}({r}) */
void RelateForEachChildIf(struct Relate *const this,
	RelatePredicate p, RelateAction a) {
	struct Relate *child;
	unsigned i;
	if(!a) return;
	for(i = 0; i < this->size; i++) {
		child = this->childs[i];
		if(!p || p(child)) a(child);
	}
}

/** E {r} \in {this.children}, !strcmp({r.key}, {key}) -> {a}({r}) */
void RelateForEachChildKey(struct Relate *const this,
	const char *const key, RelateAction a) {
	struct Relate *child;
	unsigned i;
	if(!key || !a) return;
	for(i = 0; i < this->size; i++) {
		child = this->childs[i];
		if(!strcmp(TextGet(child->key), key)) a(child);
	}
}

/** Does a short-circuit linear search of {this} for a child key that matches
 {key}.
 @return The text or null if the key was not found. */
struct Relate *RelateGetChild(const struct Relate *const this,
	const char *const key) {
	struct Relate *child, *found = 0;
	size_t d;

	if(!this || !key) return 0;
	for(d = 0; d < this->size; d++) {
		child = this->childs[d];
		if(strcmp(TextGet(child->key), key)) continue;
		found = child;
		break;
	}
	return found;
}

/** Shortcut for {TextGet(RelateGetValue(RelateGetChild(this, key)))}. */
const char *RelateGetChildValue(const struct Relate *const this,
	const char *const key) {
	return RelateValue(RelateGetChild(this, key));
}

/** Gets the parent value of {this} or null if it is root. */
struct Text *RelateGetParentValue(const struct Relate *const this) {
	if(!this || this->parent.type != T_RELATE) return 0;
	return this->parent.p.relate->value;
}



/***********
 * Private */

/** Relate filler. The errors are propagated to the {Relates}. */
static int relate(struct Relate *this, const struct Parent parent) {

	this->parent.type = parent.type;
	this->parent.p    = parent.p;

	this->key         = 0;
	this->value       = 0;

	this->size        = 0;
	this->capacity[0] = fibonacci6;
	this->capacity[1] = fibonacci7;
	this->childs      = 0;

	if(!(this->key = Text()) || !(this->value = Text())) return relate_(&this),
		to_relates(this)->error = global_error,
		to_relates(this)->errno_copy = global_errno_copy, 0;
	if(!(this->childs = malloc(this->capacity[0] * sizeof *this->childs)))
		return relate_(&this),
		to_relates(this)->error = E_ERRNO,
		to_relates(this)->errno_copy = errno, 0;

	return -1;
}

/** Relate deleter. */
static void relate_(struct Relate **const this_ptr) {
	struct Relate *this;

	if(!this_ptr || !(this = *this_ptr)) return;

	while(this->size) relate_(&this->childs[--this->size]);
	Text_(&this->value);
	Text_(&this->key);
	/* the root is included in {Relates} and should not be freed, otherwise, */
	if(this->parent.type == T_RELATE) free(this);

	*this_ptr = 0;
}

/** Spawns a new child of {this}. The errors are propagated to the {Relates}.
 @throws E_OVERFLOW, E_ERRNO */
static struct Relate *relate_new(struct Relate *this) {
	struct Relate *child;
	struct Parent parent;
	if(this->size >= this->capacity[0] && !relate_grow(this)) return 0;
	if(!(child = malloc(sizeof *child))) {
		to_relates(this)->error = E_ERRNO;
		to_relates(this)->errno_copy = errno;
		return 0;
	}
	parent.type     = T_RELATE;
	parent.p.relate = this;
	if(!relate(child, parent)) {
		to_relates(this)->error = E_ERRNO;
		to_relates(this)->errno_copy = errno;
		return 0;
	}
	this->childs[this->size++] = child;
	return child;
}

/** Grows child capacity of {this}. Should only be c
 @return Success.
 @throws E_OVERFLOW, E_ERRNO */
static int relate_grow(struct Relate *const this) {
	size_t c0, c1;
	struct Relate **childs;

	/* fibonacci */
	c0 = this->capacity[0];
	c1 = this->capacity[1];
	if(c0 == (size_t)-1) return to_relates(this)->error = E_OVERFLOW, 0;
	c0 ^= c1;
	c1 ^= c0;
	c0 ^= c1;
	c1 += c0;
	if(c1 <= c0) c1 = (size_t)-1;
	if(!(childs = realloc(this->childs, c0 * sizeof *this->childs))) return
		to_relates(this)->error = E_ERRNO,
		to_relates(this)->errno_copy = errno, 0;
	this->childs = childs;
	this->capacity[0] = c0;
	this->capacity[1] = c1;

	return -1;
}

/** @return Goes up the chain of {Relate} until it hits the {Relates}. */
static struct Relates *to_relates(struct Relate *const this) {
	const struct Relate *r;
	for(r = this; r->parent.type == T_RELATE; r = r->parent.p.relate);
	return r->parent.p.root;
}
