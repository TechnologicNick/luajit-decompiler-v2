static constexpr uint16_t BC_OP_JMP_BIAS = 0x8000;

enum BC_OP {
	BC_OP_ISLT, // if A<VAR> < D<VAR> then JMP
	BC_OP_ISGE, // if not (A<VAR> < D<VAR>) then JMP
	BC_OP_ISLE, // if A<VAR> <= D<VAR> then JMP
	BC_OP_ISGT, // if not (A<VAR> <= D<VAR>) then JMP
	BC_OP_ISEQV, // if A<VAR> == D<VAR> then JMP
	BC_OP_ISNEV, // if A<VAR> ~= D<VAR> then JMP
	BC_OP_ISEQS, // if A<VAR> == D<STR> then JMP
	BC_OP_ISNES, // if A<VAR> ~= D<STR> then JMP
	BC_OP_ISEQN, // if A<VAR> == D<NUM> then JMP
	BC_OP_ISNEN, // if A<VAR> ~= D<NUM> then JMP
	BC_OP_ISEQP, // if A<VAR> == D<PRI> then JMP
	BC_OP_ISNEP, // if A<VAR> ~= D<PRI> then JMP
	BC_OP_ISTC, // if D<VAR> then A<DST> = D and JMP
	BC_OP_ISFC, // if not D<VAR> then A<DST> = D and JMP
	BC_OP_IST, // if D<VAR> then JMP
	BC_OP_ISF, // if not D<VAR> then JMP
	BC_OP_ISTYPE, // unsupported
	BC_OP_ISNUM, // unsupported
	BC_OP_MOV, // A<DST> = D<VAR>
	BC_OP_NOT, // A<DST> = not D<VAR>
	BC_OP_UNM, // A<DST> = -D<VAR>
	BC_OP_LEN, // A<DST> = #D<VAR>
	BC_OP_ADDVN, // A<DST> = B<VAR> + C<NUM>
	BC_OP_SUBVN, // A<DST> = B<VAR> - C<NUM>
	BC_OP_MULVN, // A<DST> = B<VAR> * C<NUM>
	BC_OP_DIVVN, // A<DST> = B<VAR> / C<NUM>
	BC_OP_MODVN, // A<DST> = B<VAR> % C<NUM>
	BC_OP_ADDNV, // A<DST> = C<NUM> + B<VAR>
	BC_OP_SUBNV, // A<DST> = C<NUM> - B<VAR>
	BC_OP_MULNV, // A<DST> = C<NUM> * B<VAR>
	BC_OP_DIVNV, // A<DST> = C<NUM> / B<VAR>
	BC_OP_MODNV, // A<DST> = C<NUM> % B<VAR>
	BC_OP_ADDVV, // A<DST> = B<VAR> + C<VAR>
	BC_OP_SUBVV, // A<DST> = B<VAR> - C<VAR>
	BC_OP_MULVV, // A<DST> = B<VAR> * C<VAR>
	BC_OP_DIVVV, // A<DST> = B<VAR> / C<VAR>
	BC_OP_MODVV, // A<DST> = B<VAR> % C<VAR>
	BC_OP_POW, // A<DST> = B<VAR> ^ C<VAR>
	BC_OP_CAT, // A<DST> = B<RBASE> .. B++ -> C<RBASE>
	BC_OP_KSTR, // A<DST> = D<STR>
	BC_OP_KCDATA, // A<DST> = D<CDATA>
	BC_OP_KSHORT, // A<DST> = D<LITS>
	BC_OP_KNUM, // A<DST> = D<NUM>
	BC_OP_KPRI, // A<DST> = D<PRI>
	BC_OP_KNIL, // A<BASE>, A++ -> D<BASE> = nil
	BC_OP_UGET, // A<DST> = D<UV>
	BC_OP_USETV, // A<UV> = D<VAR>
	BC_OP_USETS, // A<UV> = D<STR>
	BC_OP_USETN, // A<UV> = D<NUM>
	BC_OP_USETP, // A<UV> = D<PRI>
	BC_OP_UCLO, // upvalue close for A<RBASE>, A++ -> framesize; goto D<JUMP>
	BC_OP_FNEW, // A<DST> = D<FUNC>
	BC_OP_TNEW, // A<DST> = {}
	BC_OP_TDUP, // A<DST> = D<TAB>
	BC_OP_GGET, // A<DST> = _G.D<STR>
	BC_OP_GSET, // _G.D<STR> = A<VAR>
	BC_OP_TGETV, // A<DST> = B<VAR>[C<VAR>]
	BC_OP_TGETS, // A<DST> = B<VAR>[C<STR>]
	BC_OP_TGETB, // A<DST> = B<VAR>[C<LIT>]
	BC_OP_TGETR, // unsupported
	BC_OP_TSETV, // B<VAR>[C<VAR>] = A<VAR>
	BC_OP_TSETS, // B<VAR>[C<STR>] = A<VAR>
	BC_OP_TSETB, // B<VAR>[C<LIT>] = A<VAR>
	BC_OP_TSETM, // A-1<BASE>[D&0xFFFFFFFF<NUM>] <- A (<- multres)
	BC_OP_TSETR, // unsupported
	BC_OP_CALLM, // if B<LIT> == 0 then A<BASE> (<- multres) <- A(A+FR2?2:1, A++ -> for C<LIT>, A++ (<- multres)) else A, A++ -> for B-1 = A(A+FR2?2:1, A++ -> for C, A++ (<- multres))
	BC_OP_CALL, // if B<LIT> == 0 then A<BASE> (<- multres) <- A(A+FR2?2:1, A++ -> for C-1<LIT>) else A, A++ -> for B-1 = A(A+FR2?2:1, A++ -> for C-1)
	BC_OP_CALLMT, // return A<BASE>(A+FR2?2:1, A++ -> for D<LIT>, A++ (<- multres))
	BC_OP_CALLT, // return A<BASE>(A+FR2?2:1, A++ -> for D-1<LIT>)
	BC_OP_ITERC, // for A<BASE>, A++ -> for B-1<LIT> in A-3, A-2, A-1 do
	BC_OP_ITERN, // for A<BASE>, A++ -> for B-1<LIT> in A-3, A-2, A-1 do
	BC_OP_VARG, // if B<LIT> == 0 then A<BASE> (<- multres) <- ... else A, A++ -> for B-1 = ...
	BC_OP_ISNEXT, // goto ITERN at D<JUMP>
	BC_OP_RETM, // return A<BASE>, A++ -> for D<LIT>, A++ (<- multres)
	BC_OP_RET, // return A<RBASE>, A++ -> for D-1<LIT>
	BC_OP_RET0, // return
	BC_OP_RET1, // return A<RBASE>
	BC_OP_FORI, // for A+3<BASE> = A, A+1, A+2 do; exit at D<JUMP>
	BC_OP_JFORI, // unsupported
	BC_OP_FORL, // end of numeric for loop; start at D<JUMP>
	BC_OP_IFORL, // unsupported
	BC_OP_JFORL, // unsupported
	BC_OP_ITERL, // end of generic for loop; start at D<JUMP>
	BC_OP_IITERL, // unsupported
	BC_OP_JITERL, // unsupported
	BC_OP_LOOP, // if D<JUMP> == 32767 then goto loop else while/repeat loop; exit at D
	BC_OP_ILOOP, // unsupported
	BC_OP_JLOOP, // unsupported
	BC_OP_JMP, // goto D<JUMP> or if true then JMP or goto ITERC at D
	BC_OP_FUNCF, // unsupported
	BC_OP_IFUNCF, // unsupported
	BC_OP_JFUNCF, // unsupported
	BC_OP_FUNCV, // unsupported
	BC_OP_IFUNCV, // unsupported
	BC_OP_JFUNCV, // unsupported
	BC_OP_FUNCC, // unsupported
	BC_OP_FUNCCW, // unsupported
	BC_OP_INVALID
};

static constexpr const char* BC_OP_NAMES[] = {
	"BC_OP_ISLT",
	"BC_OP_ISGE",
	"BC_OP_ISLE",
	"BC_OP_ISGT",
	"BC_OP_ISEQV",
	"BC_OP_ISNEV",
	"BC_OP_ISEQS",
	"BC_OP_ISNES",
	"BC_OP_ISEQN",
	"BC_OP_ISNEN",
	"BC_OP_ISEQP",
	"BC_OP_ISNEP",
	"BC_OP_ISTC",
	"BC_OP_ISFC",
	"BC_OP_IST",
	"BC_OP_ISF",
	"BC_OP_ISTYPE",
	"BC_OP_ISNUM",
	"BC_OP_MOV",
	"BC_OP_NOT",
	"BC_OP_UNM",
	"BC_OP_LEN",
	"BC_OP_ADDVN",
	"BC_OP_SUBVN",
	"BC_OP_MULVN",
	"BC_OP_DIVVN",
	"BC_OP_MODVN",
	"BC_OP_ADDNV",
	"BC_OP_SUBNV",
	"BC_OP_MULNV",
	"BC_OP_DIVNV",
	"BC_OP_MODNV",
	"BC_OP_ADDVV",
	"BC_OP_SUBVV",
	"BC_OP_MULVV",
	"BC_OP_DIVVV",
	"BC_OP_MODVV",
	"BC_OP_POW",
	"BC_OP_CAT",
	"BC_OP_KSTR",
	"BC_OP_KCDATA",
	"BC_OP_KSHORT",
	"BC_OP_KNUM",
	"BC_OP_KPRI",
	"BC_OP_KNIL",
	"BC_OP_UGET",
	"BC_OP_USETV",
	"BC_OP_USETS",
	"BC_OP_USETN",
	"BC_OP_USETP",
	"BC_OP_UCLO",
	"BC_OP_FNEW",
	"BC_OP_TNEW",
	"BC_OP_TDUP",
	"BC_OP_GGET",
	"BC_OP_GSET",
	"BC_OP_TGETV",
	"BC_OP_TGETS",
	"BC_OP_TGETB",
	"BC_OP_TGETR",
	"BC_OP_TSETV",
	"BC_OP_TSETS",
	"BC_OP_TSETB",
	"BC_OP_TSETM",
	"BC_OP_TSETR",
	"BC_OP_CALLM",
	"BC_OP_CALL",
	"BC_OP_CALLMT",
	"BC_OP_CALLT",
	"BC_OP_ITERC",
	"BC_OP_ITERN",
	"BC_OP_VARG",
	"BC_OP_ISNEXT",
	"BC_OP_RETM",
	"BC_OP_RET",
	"BC_OP_RET0",
	"BC_OP_RET1",
	"BC_OP_FORI",
	"BC_OP_JFORI",
	"BC_OP_FORL",
	"BC_OP_IFORL",
	"BC_OP_JFORL",
	"BC_OP_ITERL",
	"BC_OP_IITERL",
	"BC_OP_JITERL",
	"BC_OP_LOOP",
	"BC_OP_ILOOP",
	"BC_OP_JLOOP",
	"BC_OP_JMP",
	"BC_OP_FUNCF",
	"BC_OP_IFUNCF",
	"BC_OP_JFUNCF",
	"BC_OP_FUNCV",
	"BC_OP_IFUNCV",
	"BC_OP_JFUNCV",
	"BC_OP_FUNCC",
	"BC_OP_FUNCCW"
};

static const char* get_op_name(const BC_OP& instruction) {
	if (instruction < BC_OP_INVALID) return BC_OP_NAMES[instruction];
	return "BC_OP_INVALID";
}

static const char* get_op_description(const BC_OP& instruction) {
	switch (instruction) {
	case BC_OP_ISLT: return "if A<VAR> < D<VAR> then JMP";
	case BC_OP_ISGE: return "if not (A<VAR> < D<VAR>) then JMP";
	case BC_OP_ISLE: return "if A<VAR> <= D<VAR> then JMP";
	case BC_OP_ISGT: return "if not (A<VAR> <= D<VAR>) then JMP";
	case BC_OP_ISEQV: return "if A<VAR> == D<VAR> then JMP";
	case BC_OP_ISNEV: return "if A<VAR> ~= D<VAR> then JMP";
	case BC_OP_ISEQS: return "if A<VAR> == D<STR> then JMP";
	case BC_OP_ISNES: return "if A<VAR> ~= D<STR> then JMP";
	case BC_OP_ISEQN: return "if A<VAR> == D<NUM> then JMP";
	case BC_OP_ISNEN: return "if A<VAR> ~= D<NUM> then JMP";
	case BC_OP_ISEQP: return "if A<VAR> == D<PRI> then JMP";
	case BC_OP_ISNEP: return "if A<VAR> ~= D<PRI> then JMP";
	case BC_OP_ISTC: return "if D<VAR> then A<DST> = D and JMP";
	case BC_OP_ISFC: return "if not D<VAR> then A<DST> = D and JMP";
	case BC_OP_IST: return "if D<VAR> then JMP";
	case BC_OP_ISF: return "if not D<VAR> then JMP";
	case BC_OP_ISTYPE: return "unsupported";
	case BC_OP_ISNUM: return "unsupported";
	case BC_OP_MOV: return "A<DST> = D<VAR>";
	case BC_OP_NOT: return "A<DST> = not D<VAR>";
	case BC_OP_UNM: return "A<DST> = -D<VAR>";
	case BC_OP_LEN: return "A<DST> = #D<VAR>";
	case BC_OP_ADDVN: return "A<DST> = B<VAR> + C<NUM>";
	case BC_OP_SUBVN: return "A<DST> = B<VAR> - C<NUM>";
	case BC_OP_MULVN: return "A<DST> = B<VAR> * C<NUM>";
	case BC_OP_DIVVN: return "A<DST> = B<VAR> / C<NUM>";
	case BC_OP_MODVN: return "A<DST> = B<VAR> % C<NUM>";
	case BC_OP_ADDNV: return "A<DST> = C<NUM> + B<VAR>";
	case BC_OP_SUBNV: return "A<DST> = C<NUM> - B<VAR>";
	case BC_OP_MULNV: return "A<DST> = C<NUM> * B<VAR>";
	case BC_OP_DIVNV: return "A<DST> = C<NUM> / B<VAR>";
	case BC_OP_MODNV: return "A<DST> = C<NUM> % B<VAR>";
	case BC_OP_ADDVV: return "A<DST> = B<VAR> + C<VAR>";
	case BC_OP_SUBVV: return "A<DST> = B<VAR> - C<VAR>";
	case BC_OP_MULVV: return "A<DST> = B<VAR> * C<VAR>";
	case BC_OP_DIVVV: return "A<DST> = B<VAR> / C<VAR>";
	case BC_OP_MODVV: return "A<DST> = B<VAR> % C<VAR>";
	case BC_OP_POW: return "A<DST> = B<VAR> ^ C<VAR>";
	case BC_OP_CAT: return "A<DST> = concat B..C";
	case BC_OP_KSTR: return "A<DST> = D<STR>";
	case BC_OP_KCDATA: return "A<DST> = D<CDATA>";
	case BC_OP_KSHORT: return "A<DST> = D<LITS>";
	case BC_OP_KNUM: return "A<DST> = D<NUM>";
	case BC_OP_KPRI: return "A<DST> = D<PRI>";
	case BC_OP_KNIL: return "A<BASE>..D<BASE> = nil";
	case BC_OP_UGET: return "A<DST> = D<UV>";
	case BC_OP_USETV: return "A<UV> = D<VAR>";
	case BC_OP_USETS: return "A<UV> = D<STR>";
	case BC_OP_USETN: return "A<UV> = D<NUM>";
	case BC_OP_USETP: return "A<UV> = D<PRI>";
	case BC_OP_UCLO: return "close upvalues then goto D<JUMP>";
	case BC_OP_FNEW: return "A<DST> = D<FUNC>";
	case BC_OP_TNEW: return "A<DST> = {}";
	case BC_OP_TDUP: return "A<DST> = D<TAB>";
	case BC_OP_GGET: return "A<DST> = _G.D<STR>";
	case BC_OP_GSET: return "_G.D<STR> = A<VAR>";
	case BC_OP_TGETV: return "A<DST> = B<VAR>[C<VAR>]";
	case BC_OP_TGETS: return "A<DST> = B<VAR>[C<STR>]";
	case BC_OP_TGETB: return "A<DST> = B<VAR>[C<LIT>]";
	case BC_OP_TGETR: return "unsupported";
	case BC_OP_TSETV: return "B<VAR>[C<VAR>] = A<VAR>";
	case BC_OP_TSETS: return "B<VAR>[C<STR>] = A<VAR>";
	case BC_OP_TSETB: return "B<VAR>[C<LIT>] = A<VAR>";
	case BC_OP_TSETM: return "table mass-assign from multres";
	case BC_OP_TSETR: return "unsupported";
	case BC_OP_CALLM: return "call with multres args";
	case BC_OP_CALL: return "call";
	case BC_OP_CALLMT: return "tailcall with multres args";
	case BC_OP_CALLT: return "tailcall";
	case BC_OP_ITERC: return "generic iterator call";
	case BC_OP_ITERN: return "numeric iterator call";
	case BC_OP_VARG: return "vararg load";
	case BC_OP_ISNEXT: return "goto ITERN at D<JUMP>";
	case BC_OP_RETM: return "return multres";
	case BC_OP_RET: return "return range";
	case BC_OP_RET0: return "return";
	case BC_OP_RET1: return "return one value";
	case BC_OP_FORI: return "numeric for init";
	case BC_OP_JFORI: return "unsupported";
	case BC_OP_FORL: return "numeric for loop";
	case BC_OP_IFORL: return "unsupported";
	case BC_OP_JFORL: return "unsupported";
	case BC_OP_ITERL: return "generic for loop";
	case BC_OP_IITERL: return "unsupported";
	case BC_OP_JITERL: return "unsupported";
	case BC_OP_LOOP: return "loop dispatch";
	case BC_OP_ILOOP: return "unsupported";
	case BC_OP_JLOOP: return "unsupported";
	case BC_OP_JMP: return "jump";
	case BC_OP_FUNCF: return "unsupported";
	case BC_OP_IFUNCF: return "unsupported";
	case BC_OP_JFUNCF: return "unsupported";
	case BC_OP_FUNCV: return "unsupported";
	case BC_OP_IFUNCV: return "unsupported";
	case BC_OP_JFUNCV: return "unsupported";
	case BC_OP_FUNCC: return "unsupported";
	case BC_OP_FUNCCW: return "unsupported";
	case BC_OP_INVALID: return "invalid opcode";
	}

	return "unknown opcode";
}

struct Bytecode::Instruction {
	BC_OP type;
	uint8_t a = 0;
	uint8_t b = 0;
	uint8_t c = 0;
	uint16_t d = 0;
};

static BC_OP get_op_type(const uint8_t& byte, const uint8_t& version) {
	return (BC_OP)(version == Bytecode::BC_VERSION_1 && byte >= BC_OP_ISTYPE ? (byte >= BC_OP_TGETR - 2 ? (byte >= BC_OP_TSETR - 3 ? byte + 4 : byte + 3) : byte + 2) : byte);
}

static bool is_op_abc_format(const BC_OP& instruction) {
	switch (instruction) {
	case BC_OP_ADDVN:
	case BC_OP_SUBVN:
	case BC_OP_MULVN:
	case BC_OP_DIVVN:
	case BC_OP_MODVN:
	case BC_OP_ADDNV:
	case BC_OP_SUBNV:
	case BC_OP_MULNV:
	case BC_OP_DIVNV:
	case BC_OP_MODNV:
	case BC_OP_ADDVV:
	case BC_OP_SUBVV:
	case BC_OP_MULVV:
	case BC_OP_DIVVV:
	case BC_OP_MODVV:
	case BC_OP_POW:
	case BC_OP_CAT:
	case BC_OP_TGETV:
	case BC_OP_TGETS:
	case BC_OP_TGETB:
	case BC_OP_TGETR:
	case BC_OP_TSETV:
	case BC_OP_TSETS:
	case BC_OP_TSETB:
	case BC_OP_TSETR:
	case BC_OP_CALLM:
	case BC_OP_CALL:
	case BC_OP_ITERC:
	case BC_OP_ITERN:
	case BC_OP_VARG:
		return true;
	}

	return false;
}
