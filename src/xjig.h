
#ifndef _XJIG_H
#define _XJIG_H

enum Modifier {No_Mod = 0, RandomBit_Mod = 1, RandomIncDec_Mod = 2,
	       RandomByte_Mod = 4, RandomStretch_Mod = 8,
	       RandomAppend_Mod = 16, RandomWellFormed_Mod = 32,
	       RandomLegal_Mod = 64, RandomBogus_Mod = 128,
	       ForceCracking_Mod = 256 };

const Modifier MAX_MODIFIER = 511;

#endif
