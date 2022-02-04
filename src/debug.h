#ifndef DEBUG_H
#define DEBUG_H

/* `DEBUG` will not work on my compiler. */
#define DBG \
	X(DBG_0), \
	X(DBG_READ), \
	X(DBG_OUTPUT), X(DBG_3), \
	X(DBG_SEMANTIC), X(DBG_5), X(DBG_6), X(DBG_7), \
	X(DBG_HASH), X(DBG_9), X(DBG_10), X(DBG_11), X(DBG_12), X(DBG_13), \
	X(DBG_14), X(DBG_15), \
	X(DBG_ERASE), X(DBG_17), X(DBG_18), X(DBG_19), X(DBG_20), X(DBG_21), \
	X(DBG_22), X(DBG_23), X(DBG_24), X(DBG_25), X(DBG_26), X(DBG_27), \
	X(DBG_28), X(DBG_29), X(DBG_30), X(DBG_31), \
	X(DBG_STYLE), X(DBG_33), X(DBG_34), X(DBG_35), X(DBG_36), X(DBG_37), \
	X(DBG_38), X(DBG_39), X(DBG_40), X(DBG_41), X(DBG_42), X(DBG_43), \
	X(DBG_44), X(DBG_45), X(DBG_46), X(DBG_47), X(DBG_48), X(DBG_49), \
	X(DBG_50), X(DBG_51), X(DBG_52), X(DBG_53), X(DBG_54), X(DBG_55), \
	X(DBG_56), X(DBG_57), X(DBG_58), X(DBG_59), X(DBG_60), X(DBG_61), \
	X(DBG_62), X(DBG_63)

#define X(n) n
enum debug { DBG };
#undef X
#define X(n) #n
static const char *const debug[] = { DBG };
#undef X

#endif
