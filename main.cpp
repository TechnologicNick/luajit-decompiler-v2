#include "main.h"

struct Error {
	const std::string message;
	const std::string filePath;
	const std::string function;
	const std::string source;
	const std::string line;
};

static const HANDLE CONSOLE_OUTPUT = GetStdHandle(STD_OUTPUT_HANDLE);
//static const HANDLE CONSOLE_INPUT = GetStdHandle(STD_INPUT_HANDLE);
static bool isCommandLine;
static bool isProgressBarActive = false;
static uint32_t filesSkipped = 0;
static WORD defaultConsoleAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

static struct {
	bool showHelp = false;
	bool silentAssertions = false;
	bool forceOverwrite = false;
	bool ignoreDebugInfo = false;
	bool minimizeDiffs = false;
	bool unrestrictedAscii = false;
	bool dumpOpcodes = false;
	std::string inputPath;
	std::string outputPath;
	std::string extensionFilter;
} arguments;

struct Directory {
	const std::string path;
	std::vector<Directory> folders;
	std::vector<std::string> files;
};

static std::string string_to_lowercase(const std::string& string) {
	std::string lowercaseString = string;

	for (uint32_t i = lowercaseString.size(); i--;) {
		if (lowercaseString[i] < 'A' || lowercaseString[i] > 'Z') continue;
		lowercaseString[i] += 'a' - 'A';
	}

	return lowercaseString;
}

enum ConsoleColor : WORD {
	COLOR_DEFAULT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	COLOR_HEADER = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	COLOR_PROTOTYPE = FOREGROUND_RED | FOREGROUND_INTENSITY,
	COLOR_UPVALUE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	COLOR_OPCODE = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	COLOR_COMMENT = FOREGROUND_GREEN | FOREGROUND_BLUE,
	COLOR_VARIABLE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	COLOR_INDEX = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
};

static void write_console(const std::string& text) {
	WriteConsoleA(CONSOLE_OUTPUT, text.data(), text.size(), NULL, NULL);
}

static void set_console_color(const WORD& color) {
	SetConsoleTextAttribute(CONSOLE_OUTPUT, color);
}

static void reset_console_color() {
	set_console_color(defaultConsoleAttributes);
}

static void print_colored(const std::string& message, const WORD& color) {
	set_console_color(color);
	write_console(message);
	write_console("\n");
	reset_console_color();
}

static bool is_variable_token(const std::string& token) {
	return token.find('<') != std::string::npos
		|| token.find('[') != std::string::npos
		|| token.find("slot") != std::string::npos
		|| token.find("uv") != std::string::npos
		|| token.find("Prototype") != std::string::npos
		|| token.find("instruction") != std::string::npos
		|| token.find("_G.") != std::string::npos
		|| token.find("const_str") != std::string::npos
		|| (token.size() >= 2 && token.front() == '"' && token.back() == '"');
}

static bool is_numeric_token(const std::string& token) {
	if (!token.size()) return false;
	for (uint32_t i = 0; i < token.size(); i++) {
		if (token[i] < '0' || token[i] > '9') return false;
	}
	return true;
}

static void write_highlighted_text(const std::string& text, const WORD& baseColor, const WORD& variableColor) {
	bool previousPrototypeToken = false;

	for (uint32_t i = 0; i < text.size();) {
		if (text[i] == ' ') {
			uint32_t j = i + 1;
			while (j < text.size() && text[j] == ' ') j++;
			set_console_color(baseColor);
			write_console(text.substr(i, j - i));
			i = j;
			continue;
		}

		uint32_t j = i + 1;
		while (j < text.size() && text[j] != ' ') j++;
		const std::string token = text.substr(i, j - i);
		const bool isVariable = is_variable_token(token) || is_numeric_token(token) || (previousPrototypeToken && is_numeric_token(token));
		set_console_color(isVariable ? variableColor : baseColor);
		write_console(token);
		previousPrototypeToken = token.find("Prototype") != std::string::npos;
		i = j;
	}
}

static void print_highlighted(const std::string& message, const WORD& baseColor, const WORD& variableColor) {
	write_highlighted_text(message, baseColor, variableColor);
	write_console("\n");
	reset_console_color();
}

static void find_files_recursively(Directory& directory) {
	WIN32_FIND_DATAA pathData;
	HANDLE handle = FindFirstFileA((arguments.inputPath + directory.path + '*').c_str(), &pathData);
	if (handle == INVALID_HANDLE_VALUE) return;

	do {
		if (pathData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!std::strcmp(pathData.cFileName, ".") || !std::strcmp(pathData.cFileName, "..")) continue;
			directory.folders.emplace_back(Directory{ .path = directory.path + pathData.cFileName + "\\" });
			find_files_recursively(directory.folders.back());
			if (!directory.folders.back().files.size() && !directory.folders.back().folders.size()) directory.folders.pop_back();
			continue;
		}

		if (!arguments.extensionFilter.size() || arguments.extensionFilter == string_to_lowercase(PathFindExtensionA(pathData.cFileName))) directory.files.emplace_back(pathData.cFileName);
	} while (FindNextFileA(handle, &pathData));

	FindClose(handle);
}

static bool decompile_files_recursively(const Directory& directory) {
	CreateDirectoryA((arguments.outputPath + directory.path).c_str(), NULL);
	std::string outputFile;

	for (uint32_t i = 0; i < directory.files.size(); i++) {
		outputFile = directory.files[i];
		PathRemoveExtensionA(outputFile.data());
		outputFile = outputFile.c_str();
		outputFile += ".lua";

		Bytecode bytecode(arguments.inputPath + directory.path + directory.files[i]);
		Ast ast(bytecode, arguments.ignoreDebugInfo, arguments.minimizeDiffs);
		Lua lua(bytecode, ast, arguments.outputPath + directory.path + outputFile, arguments.forceOverwrite, arguments.minimizeDiffs, arguments.unrestrictedAscii);

		try {
			print("--------------------\nInput file: " + bytecode.filePath + "\nReading bytecode...");
			bytecode();
			print("Building ast...");
			ast();
			print("Writing lua source...");
			lua();
			print("Output file: " + lua.filePath);
		} catch (const Error& error) {
			erase_progress_bar();

			if (arguments.silentAssertions) {
				print("\nError running " + error.function + "\nSource: " + error.source + ":" + error.line + "\n\n" + error.message);
				filesSkipped++;
				continue;
			}

			switch (MessageBoxA(NULL, ("Error running " + error.function + "\nSource: " + error.source + ":" + error.line + "\n\nFile: " + error.filePath + "\n\n" + error.message).c_str(),
				PROGRAM_NAME, MB_ICONERROR | MB_CANCELTRYCONTINUE | MB_DEFBUTTON3)) {
			case IDCANCEL:
				return false;
			case IDTRYAGAIN:
				print("Retrying...");
				i--;
				continue;
			case IDCONTINUE:
				print("File skipped.");
				filesSkipped++;
			}
		} catch (...) {
			MessageBoxA(NULL, std::string("Unknown exception\n\nFile: " + bytecode.filePath).c_str(), PROGRAM_NAME, MB_ICONERROR | MB_OK);
			throw;
		}
	}

	for (uint32_t i = 0; i < directory.folders.size(); i++) {
		if (!decompile_files_recursively(directory.folders[i])) return false;
	}

	return true;
}

static std::string to_hex_16(const uint16_t& value) {
	char string[] = "0x0000";
	uint8_t digit;

	for (uint8_t i = 4; i--;) {
		digit = (value >> i * 4) & 0xF;
		string[5 - i] = digit >= 0xA ? 'A' + digit - 0xA : '0' + digit;
	}

	return string;
}

static std::string to_hex_64(const uint64_t& value) {
	char string[] = "0x0000000000000000";
	uint8_t digit;

	for (uint8_t i = 16; i--;) {
		digit = (value >> i * 4) & 0xF;
		string[17 - i] = digit >= 0xA ? 'A' + digit - 0xA : '0' + digit;
	}

	return string;
}

static std::string escape_string(const std::string& string) {
	std::string escaped;
	escaped.reserve(string.size());

	for (uint32_t i = 0; i < string.size(); i++) {
		switch (string[i]) {
		case '\\':
			escaped += "\\\\";
			break;
		case '"':
			escaped += "\\\"";
			break;
		case '\n':
			escaped += "\\n";
			break;
		case '\r':
			escaped += "\\r";
			break;
		case '\t':
			escaped += "\\t";
			break;
		default:
			escaped += string[i];
			break;
		}
	}

	return escaped;
}

static const Bytecode::Constant* get_kgc_constant(const Bytecode::Prototype* prototype, const uint16_t& index) {
	if (index >= prototype->constants.size()) return nullptr;
	return &prototype->constants[prototype->constants.size() - 1 - index];
}

static std::string describe_upvalue(const Bytecode::Prototype* prototype, const uint16_t& upvalueIndex) {
	if (upvalueIndex >= prototype->upvalues.size()) return "uv[" + std::to_string(upvalueIndex) + "]=<out_of_range>";
	const uint16_t raw = prototype->upvalues[upvalueIndex];
	const uint16_t source = raw & ~(Bytecode::BC_UV_LOCAL | Bytecode::BC_UV_IMMUTABLE);
	const bool isLocal = raw & Bytecode::BC_UV_LOCAL;
	const bool isImmutable = raw & Bytecode::BC_UV_IMMUTABLE;

	std::string description =
		"uv[" + std::to_string(upvalueIndex) + "]={"
		+ (isLocal ? "kind=parent_local_slot,source=" : "kind=parent_upvalue,source=") + std::to_string(source)
		+ ",immutable=" + std::string(isImmutable ? "true" : "false")
		+ ",raw=" + to_hex_16(raw) + "}";

	if (upvalueIndex < prototype->upvalueNames.size() && prototype->upvalueNames[upvalueIndex].size()) {
		description += " name=\"" + escape_string(prototype->upvalueNames[upvalueIndex]) + "\"";
	}

	return description;
}

static std::string describe_primitive_value(const uint16_t& value) {
	switch (value) {
	case 0:
		return "nil";
	case 1:
		return "false";
	case 2:
		return "true";
	default:
		return "<unknown_pri:" + std::to_string(value) + ">";
	}
}

static std::string describe_number_constant(const Bytecode::Prototype* prototype, const uint16_t& index) {
	if (index >= prototype->numberConstants.size()) return "<number_out_of_range:" + std::to_string(index) + ">";
	const Bytecode::NumberConstant& constant = prototype->numberConstants[index];
	switch (constant.type) {
	case Bytecode::BC_KNUM_INT:
		return std::to_string(std::bit_cast<int32_t>(constant.integer));
	case Bytecode::BC_KNUM_NUM:
		return std::to_string(std::bit_cast<double>(constant.number));
	}
	return "<number_invalid>";
}

static std::string describe_string_constant(const Bytecode::Prototype* prototype, const uint16_t& index) {
	const Bytecode::Constant* constant = get_kgc_constant(prototype, index);
	if (!constant) return "<const_out_of_range:" + std::to_string(index) + ">";
	if (constant->type != Bytecode::BC_KGC_STR) return "<const_not_string>";
	return "\"" + escape_string(constant->string) + "\"";
}

static std::string slot_ref(const uint16_t& slot) {
	return "slot[" + std::to_string(slot) + "]";
}

static std::string pad_right(const std::string& text, const uint32_t& width) {
	if (text.size() >= width) return text;
	return text + std::string(width - text.size(), ' ');
}

static std::string describe_instruction_action(
	const Bytecode::Prototype* prototype,
	const Bytecode::Instruction& instruction,
	const uint32_t& instructionIndex,
	const std::unordered_map<const Bytecode::Prototype*, uint32_t>& prototypeIds
) {
	const auto jump_target = [&instruction, &instructionIndex]() {
		return (int32_t)instructionIndex + ((int32_t)instruction.d - (int32_t)Bytecode::BC_OP_JMP_BIAS + 1);
	};

	switch (instruction.type) {
	case Bytecode::BC_OP_ISLT:
		return "if " + slot_ref(instruction.a) + " < " + slot_ref(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISGE:
		return "if " + slot_ref(instruction.a) + " >= " + slot_ref(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISLE:
		return "if " + slot_ref(instruction.a) + " <= " + slot_ref(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISGT:
		return "if " + slot_ref(instruction.a) + " > " + slot_ref(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISEQV:
		return "if " + slot_ref(instruction.a) + " == " + slot_ref(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISNEV:
		return "if " + slot_ref(instruction.a) + " ~= " + slot_ref(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISEQS:
		return "if " + slot_ref(instruction.a) + " == " + describe_string_constant(prototype, instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISNES:
		return "if " + slot_ref(instruction.a) + " ~= " + describe_string_constant(prototype, instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISEQN:
		return "if " + slot_ref(instruction.a) + " == " + describe_number_constant(prototype, instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISNEN:
		return "if " + slot_ref(instruction.a) + " ~= " + describe_number_constant(prototype, instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISEQP:
		return "if " + slot_ref(instruction.a) + " == " + describe_primitive_value(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISNEP:
		return "if " + slot_ref(instruction.a) + " ~= " + describe_primitive_value(instruction.d) + " then execute next, else jump";
	case Bytecode::BC_OP_ISTC:
		return "if " + slot_ref(instruction.d) + " is truthy, copy it to " + slot_ref(instruction.a) + " and skip next instruction";
	case Bytecode::BC_OP_ISFC:
		return "if " + slot_ref(instruction.d) + " is falsy, copy it to " + slot_ref(instruction.a) + " and skip next instruction";
	case Bytecode::BC_OP_IST:
		return "if " + slot_ref(instruction.d) + " is truthy, skip next instruction";
	case Bytecode::BC_OP_ISF:
		return "if " + slot_ref(instruction.d) + " is falsy, skip next instruction";
	case Bytecode::BC_OP_ISTYPE:
		return "unsupported type-test opcode encountered";
	case Bytecode::BC_OP_ISNUM:
		return "unsupported numeric-test opcode encountered";
	case Bytecode::BC_OP_MOV:
		return "copy " + slot_ref(instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_NOT:
		return "compute logical NOT of " + slot_ref(instruction.d) + " and store in " + slot_ref(instruction.a);
	case Bytecode::BC_OP_UNM:
		return "negate " + slot_ref(instruction.d) + " and store in " + slot_ref(instruction.a);
	case Bytecode::BC_OP_LEN:
		return "take length of " + slot_ref(instruction.d) + " and store in " + slot_ref(instruction.a);
	case Bytecode::BC_OP_ADDVN:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " + " + describe_number_constant(prototype, instruction.c);
	case Bytecode::BC_OP_SUBVN:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " - " + describe_number_constant(prototype, instruction.c);
	case Bytecode::BC_OP_MULVN:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " * " + describe_number_constant(prototype, instruction.c);
	case Bytecode::BC_OP_DIVVN:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " / " + describe_number_constant(prototype, instruction.c);
	case Bytecode::BC_OP_MODVN:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " % " + describe_number_constant(prototype, instruction.c);
	case Bytecode::BC_OP_ADDNV:
		return slot_ref(instruction.a) + " = " + describe_number_constant(prototype, instruction.c) + " + " + slot_ref(instruction.b);
	case Bytecode::BC_OP_SUBNV:
		return slot_ref(instruction.a) + " = " + describe_number_constant(prototype, instruction.c) + " - " + slot_ref(instruction.b);
	case Bytecode::BC_OP_MULNV:
		return slot_ref(instruction.a) + " = " + describe_number_constant(prototype, instruction.c) + " * " + slot_ref(instruction.b);
	case Bytecode::BC_OP_DIVNV:
		return slot_ref(instruction.a) + " = " + describe_number_constant(prototype, instruction.c) + " / " + slot_ref(instruction.b);
	case Bytecode::BC_OP_MODNV:
		return slot_ref(instruction.a) + " = " + describe_number_constant(prototype, instruction.c) + " % " + slot_ref(instruction.b);
	case Bytecode::BC_OP_ADDVV:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " + " + slot_ref(instruction.c);
	case Bytecode::BC_OP_SUBVV:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " - " + slot_ref(instruction.c);
	case Bytecode::BC_OP_MULVV:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " * " + slot_ref(instruction.c);
	case Bytecode::BC_OP_DIVVV:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " / " + slot_ref(instruction.c);
	case Bytecode::BC_OP_MODVV:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " % " + slot_ref(instruction.c);
	case Bytecode::BC_OP_POW:
		return slot_ref(instruction.a) + " = " + slot_ref(instruction.b) + " ^ " + slot_ref(instruction.c);
	case Bytecode::BC_OP_CAT:
		return "concatenate " + slot_ref(instruction.b) + " through " + slot_ref(instruction.c) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_KSTR:
		return "load string constant " + describe_string_constant(prototype, instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_KCDATA:
		return "load cdata constant index " + std::to_string(instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_KSHORT:
		return "load signed literal " + std::to_string(std::bit_cast<int16_t>(instruction.d)) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_KNUM:
		return "load numeric constant " + describe_number_constant(prototype, instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_KPRI:
		return "load primitive " + describe_primitive_value(instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_KNIL:
		return "set " + slot_ref(instruction.a) + " through " + slot_ref(instruction.d) + " to nil";
	case Bytecode::BC_OP_UGET:
		return "read upvalue " + describe_upvalue(prototype, instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_USETV:
		return "write " + slot_ref(instruction.d) + " into upvalue " + describe_upvalue(prototype, instruction.a);
	case Bytecode::BC_OP_USETS:
		return "write string " + describe_string_constant(prototype, instruction.d) + " into upvalue " + describe_upvalue(prototype, instruction.a);
	case Bytecode::BC_OP_USETN:
		return "write number " + describe_number_constant(prototype, instruction.d) + " into upvalue " + describe_upvalue(prototype, instruction.a);
	case Bytecode::BC_OP_USETP:
		return "write primitive " + describe_primitive_value(instruction.d) + " into upvalue " + describe_upvalue(prototype, instruction.a);
	case Bytecode::BC_OP_UCLO:
		if (instruction.d == Bytecode::BC_OP_JMP_BIAS) return "finalize captured parent locals from " + slot_ref(instruction.a) + " upward, then execute the next instruction";
		return "finalize captured parent locals from " + slot_ref(instruction.a) + " upward, then jump to instruction " + std::to_string(jump_target());
	case Bytecode::BC_OP_FNEW: {
		const Bytecode::Constant* constant = get_kgc_constant(prototype, instruction.d);
		if (!constant || constant->type != Bytecode::BC_KGC_CHILD || !constant->prototype) {
			return "create closure from child prototype <invalid> and store in " + slot_ref(instruction.a);
		}
		const auto id = prototypeIds.find(constant->prototype);
		const std::string target = id == prototypeIds.end() ? "<unassigned>" : ("Prototype " + std::to_string(id->second));
		return "create closure from " + target + " and store in " + slot_ref(instruction.a);
	}
	case Bytecode::BC_OP_TNEW:
		return "create a new table and store it in " + slot_ref(instruction.a);
	case Bytecode::BC_OP_TDUP:
		return "duplicate table constant index " + std::to_string(instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_GGET:
		return "read global " + describe_string_constant(prototype, instruction.d) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_GSET:
		return "write " + slot_ref(instruction.a) + " to global " + describe_string_constant(prototype, instruction.d);
	case Bytecode::BC_OP_TGETV:
		return "read " + slot_ref(instruction.b) + "[" + slot_ref(instruction.c) + "] into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_TGETS:
		return "read table key " + describe_string_constant(prototype, instruction.c) + " from " + slot_ref(instruction.b) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_TGETB:
		return "read literal key " + std::to_string(instruction.c) + " from " + slot_ref(instruction.b) + " into " + slot_ref(instruction.a);
	case Bytecode::BC_OP_TGETR:
		return "unsupported table-read opcode encountered";
	case Bytecode::BC_OP_TSETV:
		return "write " + slot_ref(instruction.a) + " to " + slot_ref(instruction.b) + "[" + slot_ref(instruction.c) + "]";
	case Bytecode::BC_OP_TSETS:
		return "write " + slot_ref(instruction.a) + " to table key " + describe_string_constant(prototype, instruction.c) + " in " + slot_ref(instruction.b);
	case Bytecode::BC_OP_TSETB:
		return "write " + slot_ref(instruction.a) + " to literal key " + std::to_string(instruction.c) + " in " + slot_ref(instruction.b);
	case Bytecode::BC_OP_TSETM:
		return "bulk-write multres values into table base near " + slot_ref(instruction.a);
	case Bytecode::BC_OP_TSETR:
		return "unsupported table-write opcode encountered";
	case Bytecode::BC_OP_CALLM:
		return "call function in " + slot_ref(instruction.a) + " with multres arguments/results";
	case Bytecode::BC_OP_CALL:
		return "call function in " + slot_ref(instruction.a) + " with encoded argument/result counts";
	case Bytecode::BC_OP_CALLMT:
		return "tail-call function in " + slot_ref(instruction.a) + " with multres arguments";
	case Bytecode::BC_OP_CALLT:
		return "tail-call function in " + slot_ref(instruction.a);
	case Bytecode::BC_OP_ITERC:
		return "perform generic-iterator call using base " + slot_ref(instruction.a);
	case Bytecode::BC_OP_ITERN:
		return "perform numeric-iterator call using base " + slot_ref(instruction.a);
	case Bytecode::BC_OP_VARG:
		return "load vararg values into registers starting at " + slot_ref(instruction.a);
	case Bytecode::BC_OP_ISNEXT:
		return "check isnext fast-path and jump to instruction " + std::to_string(jump_target());
	case Bytecode::BC_OP_RETM:
		return "return values from " + slot_ref(instruction.a) + " including multres tail";
	case Bytecode::BC_OP_RET:
		return "return " + std::to_string(instruction.d - 1) + " value(s) starting at " + slot_ref(instruction.a);
	case Bytecode::BC_OP_RET0:
		return "return with no values";
	case Bytecode::BC_OP_RET1:
		return "return " + slot_ref(instruction.a);
	case Bytecode::BC_OP_FORI:
		return "initialize numeric for-loop state at " + slot_ref(instruction.a) + " with exit jump to instruction " + std::to_string(jump_target());
	case Bytecode::BC_OP_JFORI:
		return "unsupported JFORI opcode encountered";
	case Bytecode::BC_OP_FORL:
		return "advance numeric for-loop and jump back to instruction " + std::to_string(jump_target());
	case Bytecode::BC_OP_IFORL:
		return "unsupported IFORL opcode encountered";
	case Bytecode::BC_OP_JFORL:
		return "unsupported JFORL opcode encountered";
	case Bytecode::BC_OP_ITERL:
		return "advance generic for-loop and jump back to instruction " + std::to_string(jump_target());
	case Bytecode::BC_OP_IITERL:
		return "unsupported IITERL opcode encountered";
	case Bytecode::BC_OP_JITERL:
		return "unsupported JITERL opcode encountered";
	case Bytecode::BC_OP_LOOP:
		return "loop dispatcher jump target is instruction " + std::to_string(jump_target());
	case Bytecode::BC_OP_ILOOP:
		return "unsupported ILOOP opcode encountered";
	case Bytecode::BC_OP_JLOOP:
		return "unsupported JLOOP opcode encountered";
	case Bytecode::BC_OP_JMP:
		return "unconditional jump to instruction " + std::to_string(jump_target());
	case Bytecode::BC_OP_FUNCF:
		return "unsupported FUNCF opcode encountered";
	case Bytecode::BC_OP_IFUNCF:
		return "unsupported IFUNCF opcode encountered";
	case Bytecode::BC_OP_JFUNCF:
		return "unsupported JFUNCF opcode encountered";
	case Bytecode::BC_OP_FUNCV:
		return "unsupported FUNCV opcode encountered";
	case Bytecode::BC_OP_IFUNCV:
		return "unsupported IFUNCV opcode encountered";
	case Bytecode::BC_OP_JFUNCV:
		return "unsupported JFUNCV opcode encountered";
	case Bytecode::BC_OP_FUNCC:
		return "unsupported FUNCC opcode encountered";
	case Bytecode::BC_OP_FUNCCW:
		return "unsupported FUNCCW opcode encountered";
	case Bytecode::BC_OP_INVALID:
		return "invalid opcode marker";
	}

	return "unknown opcode";
}

static void assign_prototype_ids(
	const Bytecode::Prototype* prototype,
	std::unordered_map<const Bytecode::Prototype*, uint32_t>& prototypeIds,
	uint32_t& nextPrototypeId
) {
	if (prototypeIds.find(prototype) != prototypeIds.end()) return;
	prototypeIds[prototype] = nextPrototypeId++;

	for (uint32_t i = 0; i < prototype->constants.size(); i++) {
		if (prototype->constants[i].type != Bytecode::BC_KGC_CHILD || !prototype->constants[i].prototype) continue;
		assign_prototype_ids(prototype->constants[i].prototype, prototypeIds, nextPrototypeId);
	}
}

static void dump_disassembly_from_prototype(
	const Bytecode::Prototype* prototype,
	const std::unordered_map<const Bytecode::Prototype*, uint32_t>& prototypeIds
) {
	const uint32_t prototypeId = prototypeIds.at(prototype);
	print_colored("Prototype " + std::to_string(prototypeId)
		+ " header: params=" + std::to_string(prototype->header.parameters)
		+ " framesize=" + std::to_string(prototype->header.framesize)
		+ " upvalues=" + std::to_string(prototype->upvalues.size())
		+ " instructions=" + std::to_string(prototype->instructions.size()), COLOR_PROTOTYPE);

	if (!prototype->upvalues.size()) {
		print_highlighted("  Upvalues: <none>", COLOR_UPVALUE, COLOR_VARIABLE);
	} else {
		print_highlighted("  Upvalues:", COLOR_UPVALUE, COLOR_VARIABLE);
		for (uint16_t i = 0; i < prototype->upvalues.size(); i++) {
			print_highlighted("    [" + std::to_string(i) + "] " + describe_upvalue(prototype, i), COLOR_UPVALUE, COLOR_VARIABLE);
		}
	}

	print_colored("  Constants:", COLOR_HEADER);
	if (!prototype->constants.size()) {
		print_highlighted("    " + pad_right("KGC: <none>", 26) + " ; GC constants (strings/tables/child prototypes/cdata), indexed by D/C operands", COLOR_COMMENT, COLOR_VARIABLE);
	} else {
		print_highlighted("    " + pad_right("KGC (operand-indexed):", 26) + " ; GC constants (strings/tables/child prototypes/cdata), indexed by D/C operands", COLOR_COMMENT, COLOR_VARIABLE);
		for (uint16_t i = 0; i < prototype->constants.size(); i++) {
			const Bytecode::Constant* constant = get_kgc_constant(prototype, i);
			std::string typeName;
			std::string payload;

			switch (constant->type) {
			case Bytecode::BC_KGC_CHILD: {
				const auto id = prototypeIds.find(constant->prototype);
				typeName = "CHILD";
				payload = id == prototypeIds.end() ? "<unassigned>" : ("Prototype " + std::to_string(id->second));
				break;
			}
			case Bytecode::BC_KGC_TAB:
				typeName = "TAB";
				payload = "array=" + std::to_string(constant->array.size()) + " hash=" + std::to_string(constant->table.size());
				break;
			case Bytecode::BC_KGC_I64:
				typeName = "I64";
				payload = to_hex_64(constant->cdata);
				break;
			case Bytecode::BC_KGC_U64:
				typeName = "U64";
				payload = to_hex_64(constant->cdata);
				break;
			case Bytecode::BC_KGC_COMPLEX:
				typeName = "COMPLEX";
				payload = to_hex_64(constant->cdata);
				break;
			case Bytecode::BC_KGC_STR:
				typeName = "STR";
				payload = "\"" + escape_string(constant->string) + "\"";
				break;
			}
			
			set_console_color(COLOR_COMMENT);
			write_console("      ");
			set_console_color(COLOR_INDEX);
			write_console("[" + std::to_string(i) + "]");
			write_console(" ");
			set_console_color(COLOR_OPCODE);
			write_console(pad_right(typeName, 7));
			write_console(" ");
			set_console_color(COLOR_COMMENT);
			write_highlighted_text(payload, COLOR_COMMENT, COLOR_VARIABLE);
			write_console("\n");
			reset_console_color();
		}
	}

	if (!prototype->numberConstants.size()) {
		print_highlighted("    " + pad_right("KNUM: <none>", 26) + " ; numeric constants (INT/NUM), indexed directly by numeric operands", COLOR_COMMENT, COLOR_VARIABLE);
	} else {
		print_highlighted("    " + pad_right("KNUM:", 26) + " ; numeric constants (INT/NUM), indexed directly by numeric operands", COLOR_COMMENT, COLOR_VARIABLE);
		for (uint16_t i = 0; i < prototype->numberConstants.size(); i++) {
			std::string line = "      [" + std::to_string(i) + "] ";
			const Bytecode::NumberConstant& constant = prototype->numberConstants[i];
			if (constant.type == Bytecode::BC_KNUM_INT) {
				line += "INT " + std::to_string(std::bit_cast<int32_t>(constant.integer));
			} else {
				line += "NUM " + std::to_string(std::bit_cast<double>(constant.number));
			}
			print_highlighted(line, COLOR_COMMENT, COLOR_VARIABLE);
		}
	}

	print_colored("  Instructions:", COLOR_HEADER);
	for (uint32_t i = 0; i < prototype->instructions.size(); i++) {
		const Bytecode::Instruction& instruction = prototype->instructions[i];
		static constexpr uint32_t INDEX_WIDTH = 4;
		static constexpr uint32_t OPCODE_WIDTH = 14;
		static constexpr uint32_t OPERANDS_WIDTH = 12;
		std::string operands;

		if (Bytecode::is_op_abc_format(instruction.type)) {
			operands = "A=" + std::to_string(instruction.a) + " B=" + std::to_string(instruction.b) + " C=" + std::to_string(instruction.c);
		} else {
			operands = "A=" + std::to_string(instruction.a) + " D=" + std::to_string(instruction.d);
		}

		write_console("    " + pad_right("[" + std::to_string(i) + "]", INDEX_WIDTH) + " ");
		set_console_color(COLOR_OPCODE);
		write_console(pad_right(Bytecode::get_op_name(instruction.type), OPCODE_WIDTH));
		reset_console_color();
		write_console(" " + pad_right(operands, OPERANDS_WIDTH));

		const std::string symbolicDescription = Bytecode::get_op_description(instruction.type);
		const std::string semanticDescription = describe_instruction_action(prototype, instruction, i, prototypeIds);
		static constexpr uint32_t SYMBOLIC_WIDTH = 42;
		static constexpr uint32_t SEMANTIC_WIDTH = 96;
		set_console_color(COLOR_COMMENT);
		write_console(" ; ");
		write_highlighted_text(pad_right(symbolicDescription, SYMBOLIC_WIDTH), COLOR_COMMENT, COLOR_VARIABLE);
		write_console(" | ");
		write_highlighted_text(pad_right(semanticDescription, SEMANTIC_WIDTH), COLOR_COMMENT, COLOR_VARIABLE);
		write_console("\n");
		reset_console_color();
	}

	for (uint32_t i = 0; i < prototype->constants.size(); i++) {
		if (prototype->constants[i].type != Bytecode::BC_KGC_CHILD || !prototype->constants[i].prototype) continue;
		dump_disassembly_from_prototype(prototype->constants[i].prototype, prototypeIds);
	}
}

static bool dump_opcodes_in_file(const std::string& filePath) {
	try {
		Bytecode bytecode(filePath, true);
		bytecode();
		std::unordered_map<const Bytecode::Prototype*, uint32_t> prototypeIds;
		uint32_t nextPrototypeId = 0;
		assign_prototype_ids(bytecode.main, prototypeIds, nextPrototypeId);
		print_colored("--------------------\nInput file: " + filePath + "\nDisassembly:", COLOR_COMMENT);
		dump_disassembly_from_prototype(bytecode.main, prototypeIds);
	} catch (const Error& error) {
		print("\nError running " + error.function + "\nSource: " + error.source + ":" + error.line + "\n\nFile: " + error.filePath + "\n\n" + error.message);
		return false;
	}

	return true;
}

static bool dump_opcodes_recursively(const Directory& directory) {
	for (uint32_t i = 0; i < directory.files.size(); i++) {
		if (!dump_opcodes_in_file(arguments.inputPath + directory.path + directory.files[i])) return false;
	}

	for (uint32_t i = 0; i < directory.folders.size(); i++) {
		if (!dump_opcodes_recursively(directory.folders[i])) return false;
	}

	return true;
}

static char* parse_arguments(const int& argc, char** const& argv) {
	if (argc < 2) return nullptr;
	arguments.inputPath = argv[1];
#ifndef _DEBUG
	if (!isCommandLine) return nullptr;
#endif
	bool isInputPathSet = true;

	if (arguments.inputPath.size() && arguments.inputPath.front() == '-') {
		arguments.inputPath.clear();
		isInputPathSet = false;
	}

	std::string argument;

	for (uint32_t i = isInputPathSet ? 2 : 1; i < argc; i++) {
		argument = argv[i];

		if (argument.size() >= 2 && argument.front() == '-') {
			if (argument[1] == '-') {
				argument = argument.c_str() + 2;

				if (argument == "extension") {
					if (i <= argc - 2) {
						i++;
						arguments.extensionFilter = argv[i];
						continue;
					}
				} else if (argument == "force_overwrite") {
					arguments.forceOverwrite = true;
					continue;
				} else if (argument == "help") {
					arguments.showHelp = true;
					continue;
				} else if (argument == "ignore_debug_info") {
					arguments.ignoreDebugInfo = true;
					continue;
				} else if (argument == "minimize_diffs") {
					arguments.minimizeDiffs = true;
					continue;
				} else if (argument == "output") {
					if (i <= argc - 2) {
						i++;
						arguments.outputPath = argv[i];
						continue;
					}
				} else if (argument == "silent_assertions") {
					arguments.silentAssertions = true;
					continue;
				} else if (argument == "dump_opcodes") {
					arguments.dumpOpcodes = true;
					continue;
				} else if (argument == "unrestricted_ascii") {
					arguments.unrestrictedAscii = true;
					continue;
				}
			} else if (argument.size() == 2) {
				switch (argument[1]) {
				case 'e':
					if (i > argc - 2) break;
					i++;
					arguments.extensionFilter = argv[i];
					continue;
				case 'f':
					arguments.forceOverwrite = true;
					continue;
				case '?':
				case 'h':
					arguments.showHelp = true;
					continue;
				case 'i':
					arguments.ignoreDebugInfo = true;
					continue;
				case 'm':
					arguments.minimizeDiffs = true;
					continue;
				case 'o':
					if (i > argc - 2) break;
					i++;
					arguments.outputPath = argv[i];
					continue;
				case 's':
					arguments.silentAssertions = true;
					continue;
				case 'd':
					arguments.dumpOpcodes = true;
					continue;
				case 'u':
					arguments.unrestrictedAscii = true;
					continue;
				}
			}
		}

		return argv[i];
	}

	return nullptr;
}

static void wait_for_exit() {
	if (isCommandLine) return;
	print("Press any key to exit.");

	while (!_kbhit()) {
		Sleep(0);
	};
}

int main(int argc, char* argv[]) {
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	if (GetConsoleScreenBufferInfo(CONSOLE_OUTPUT, &consoleInfo)) {
		defaultConsoleAttributes = consoleInfo.wAttributes;
	}

	{
		HWND window = GetConsoleWindow();
		DWORD consoleProcessId;
		GetWindowThreadProcessId(window, &consoleProcessId);
#ifdef _DEBUG
		isCommandLine = false;
#else
		isCommandLine = consoleProcessId != GetCurrentProcessId();
		if (!isCommandLine) SetWindowTextA(window, PROGRAM_NAME);
#endif
	}

	print(std::string(PROGRAM_NAME) + "\nCompiled on " + __DATE__);
	
	char* invalidArgument = parse_arguments(argc, argv);
	if (invalidArgument) {
		print("Invalid argument: " + std::string(invalidArgument) + "\nUse -? to show usage and options.");
		return EXIT_FAILURE;
	}
	
	if (arguments.showHelp) {
		print(
			"Usage: luajit-decompiler-v2.exe INPUT_PATH [options]\n"
			"\n"
			"Available options:\n"
			"  -h, -?, --help\t\tShow this message\n"
			"  -o, --output OUTPUT_PATH\tOverride default output directory\n"
			"  -e, --extension EXTENSION\tOnly decompile files with the specified extension\n"
			"  -s, --silent_assertions\tDisable assertion error pop-up window\n"
			"\t\t\t\t  and auto skip files that fail to decompile\n"
			"  -f, --force_overwrite\t\tAlways overwrite existing files\n"
			"  -i, --ignore_debug_info\tIgnore bytecode debug info\n"
			"  -m, --minimize_diffs\t\tOptimize output formatting to help minimize diffs\n"
			"  -d, --dump_opcodes\t\tDisassemble input prototype(s) and operands\n"
			"  -u, --unrestricted_ascii\tDisable default UTF-8 encoding and string restrictions"
		);
		return EXIT_SUCCESS;
	}
	
	if (!arguments.inputPath.size()) {
		print("No input path specified!");
		if (isCommandLine) return EXIT_FAILURE;
		arguments.inputPath.resize(MAX_PATH, NULL);
		OPENFILENAMEA dialogInfo = {
			.lStructSize = sizeof(OPENFILENAMEA),
			.hwndOwner = NULL,
			.lpstrFilter = NULL,
			.lpstrCustomFilter = NULL,
			.lpstrFile = arguments.inputPath.data(),
			.nMaxFile = (DWORD)arguments.inputPath.size(),
			.lpstrFileTitle = NULL,
			.lpstrInitialDir = NULL,
			.lpstrTitle = PROGRAM_NAME,
			.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
			.lpstrDefExt = NULL,
			.FlagsEx = NULL
		};
		print("Please select a valid LuaJIT bytecode file.");
		if (!GetOpenFileNameA(&dialogInfo)) return EXIT_FAILURE;
		arguments.inputPath = arguments.inputPath.c_str();
	}

	DWORD pathAttributes;

	if (arguments.extensionFilter.size()) {
		if (arguments.extensionFilter.front() != '.') arguments.extensionFilter.insert(arguments.extensionFilter.begin(), '.');
		arguments.extensionFilter = string_to_lowercase(arguments.extensionFilter);
	}

	pathAttributes = GetFileAttributesA(arguments.inputPath.c_str());

	if (pathAttributes == INVALID_FILE_ATTRIBUTES) {
		print("Failed to open input path: " + arguments.inputPath);
		wait_for_exit();
		return EXIT_FAILURE;
	}

	Directory root;

	if (pathAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		switch (arguments.inputPath.back()) {
		case '/':
		case '\\':
			break;
		default:
			arguments.inputPath += '\\';
			break;
		}

		find_files_recursively(root);

		if (!root.files.size() && !root.folders.size()) {
			print("No files " + (arguments.extensionFilter.size() ? "with extension " + arguments.extensionFilter + " " : "") + "found in path: " + arguments.inputPath);
			wait_for_exit();
			return EXIT_FAILURE;
		}
	} else {
		root.files.emplace_back(PathFindFileNameA(arguments.inputPath.c_str()));
		*PathFindFileNameA(arguments.inputPath.c_str()) = '\x00';
		arguments.inputPath = arguments.inputPath.c_str();
	}

	if (arguments.dumpOpcodes) {
		if (!dump_opcodes_recursively(root)) return EXIT_FAILURE;
		return EXIT_SUCCESS;
	}

	if (!arguments.outputPath.size()) {
		arguments.outputPath.resize(MAX_PATH);
		GetModuleFileNameA(NULL, arguments.outputPath.data(), arguments.outputPath.size());
		*PathFindFileNameA(arguments.outputPath.data()) = '\x00';
		arguments.outputPath = arguments.outputPath.c_str();
		arguments.outputPath += "output\\";
		arguments.outputPath.shrink_to_fit();
	} else {
		pathAttributes = GetFileAttributesA(arguments.outputPath.c_str());

		if (pathAttributes == INVALID_FILE_ATTRIBUTES) {
			print("Failed to open output path: " + arguments.outputPath);
			return EXIT_FAILURE;
		}

		if (!(pathAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			print("Output path is not a folder!");
			return EXIT_FAILURE;
		}

		switch (arguments.outputPath.back()) {
		case '/':
		case '\\':
			break;
		default:
			arguments.outputPath += '\\';
			break;
		}
	}

	try {
		if (!decompile_files_recursively(root)) {
			print("--------------------\nAborted!");
			wait_for_exit();
			return EXIT_FAILURE;
		}
	} catch (...) {
		throw;
	}

#ifndef _DEBUG
	print("--------------------\n" + (filesSkipped ? "Failed to decompile " + std::to_string(filesSkipped) + " file" + (filesSkipped > 1 ? "s" : "") + ".\n" : "") + "Done!");
	wait_for_exit();
#endif
	return EXIT_SUCCESS;
}

void print(const std::string& message) {
	WriteConsoleA(CONSOLE_OUTPUT, (message + '\n').data(), message.size() + 1, NULL, NULL);
}

/*
std::string input() {
	static char BUFFER[1024];

	FlushConsoleInputBuffer(CONSOLE_INPUT);
	DWORD charsRead;
	return ReadConsoleA(CONSOLE_INPUT, BUFFER, sizeof(BUFFER), &charsRead, NULL) && charsRead > 2 ? std::string(BUFFER, charsRead - 2) : "";
}
*/

void print_progress_bar(const double& progress, const double& total) {
	static char PROGRESS_BAR[] = "\r[====================]";

	const uint8_t threshold = std::round(20 / total * progress);

	for (uint8_t i = 20; i--;) {
		PROGRESS_BAR[i + 2] = i < threshold ? '=' : ' ';
	}

	WriteConsoleA(CONSOLE_OUTPUT, PROGRESS_BAR, sizeof(PROGRESS_BAR) - 1, NULL, NULL);
	isProgressBarActive = true;
}

void erase_progress_bar() {
	static constexpr char PROGRESS_BAR_ERASER[] = "\r                      \r";

	if (!isProgressBarActive) return;
	WriteConsoleA(CONSOLE_OUTPUT, PROGRESS_BAR_ERASER, sizeof(PROGRESS_BAR_ERASER) - 1, NULL, NULL);
	isProgressBarActive = false;
}

void assert(const bool& assertion, const std::string& message, const std::string& filePath, const std::string& function, const std::string& source, const uint32_t& line) {
	if (!assertion) throw Error{
		.message = message,
		.filePath = filePath,
		.function = function,
		.source = source,
		.line = std::to_string(line)
	};
}

std::string byte_to_string(const uint8_t& byte) {
	char string[] = "0x00";
	uint8_t digit;
	
	for (uint8_t i = 2; i--;) {
		digit = (byte >> i * 4) & 0xF;
		string[3 - i] = digit >= 0xA ? 'A' + digit - 0xA : '0' + digit;
	}

	return string;
}

