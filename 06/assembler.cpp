#include <iostream>
#include <string> 
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <cctype>


using namespace std;

bool endsWith(string s1, string s2) {
    int s1_length = s1.length();
    if(s1_length < 5) { //x.asm must be at least 5 characters
        cerr << "x.asm must be at least 5 characters\n";
        exit(2);
    }
    string s1_end = s1.substr(s1_length - 4, 4);

    return (s1_end == ".asm");
}

class Parser {
private:
    enum COMMAND_TYPE {
        A_COMMAND = 0,
        C_COMMAND,
        L_COMMAND
    };

    ifstream asm_fs;

    string current_command;
    int current_index;

    map<string, int> symbol_table;
    vector<string> stripped_commands;
    vector<string> translated_commands;

    bool hasMoreCommands();
    void advance();
    COMMAND_TYPE commandType();

    string symbol();
    string dest();
    string comp();
    string jump();

    void translateAcommand();
    void translateCcommand();
    string toBinary(int n);
    void stripInput();

    void resetCurrent();
    void processSymbolFree();
    void convertToSymbolFree();
        void initialization();
        void first_pass();
        void second_pass();
            bool current_is_symbolic();

public:
    void parse();
    Parser(string input_file);

    void printTranslatedCommands();


};

void Parser::resetCurrent() {
    current_index = -1;
    current_command = "";
}

void Parser::stripInput() {
    string cur_line;

    vector<string> prestripped_commands;

    //remove empty lines, whitespace lines, and leftmost whitespace (for nonempty/nonwhitespace lines)
    while(getline(asm_fs, cur_line)) {
        //empty line
        if(cur_line.length() == 1) {
            continue;
        }
        //find index of first non-whitespace character
        int i = 0; 
        int index = 0;
        bool foundNonWhite = false;
        while(i < cur_line.length() - 1) {
            char curChar = cur_line.at(i);
            if( (curChar != ' ') && (curChar != '\t') ) {
                foundNonWhite = true;
                index = i;
                break;
            }
            i++;
        }
        if(foundNonWhite) {
            prestripped_commands.push_back(cur_line.substr(index, cur_line.length() - index));
        }
    }
    
    //now search every line for "//", and remove that onward. if // is found at 0, don't add the line
    for(int i = 0; i < prestripped_commands.size(); i++) {
        string s = prestripped_commands[i];
        int index = s.find("//");
        //we will add the line if the index wasn't 0, because we found it on the right side or because we
        //didn't find "//" at all
        if(index < 0) {
            stripped_commands.push_back(s);
        }
        else if(index > 0) {
            stripped_commands.push_back(s.substr(0, index));
        }
        //if index was 0, we found // at the beginning and so we discard the whole line (by not adding it)
    }

    //remove white space between beginning of where comment was and end of first word
    //for ex, the line "@0  //" would have left "@0  "
    for(int i = 0; i < stripped_commands.size(); i++) {
        stringstream ss(stripped_commands[i]);
        ss >> stripped_commands[i];
    }
}

//format: dest=comp
string Parser::dest() {
    string dest;
    
    int equalsIndex = current_command.find("=");
    if(equalsIndex < 0) { //did not find =, so dest bits are unused
        dest = "null";
    }
    else {
        dest = current_command.substr(0, equalsIndex);
    }

    //string::compare returns 0 on an equal comparison
    string dest_bits;
    if(!dest.compare("null"))       dest_bits = "000";
    else if(!dest.compare("M"))     dest_bits = "001";
    else if(!dest.compare("D"))     dest_bits = "010";
    else if(!dest.compare("MD"))    dest_bits = "011";
    else if(!dest.compare("A"))     dest_bits = "100";
    else if(!dest.compare("AM"))    dest_bits = "101";
    else if(!dest.compare("AD"))    dest_bits = "110";
    else if(!dest.compare("AMD"))   dest_bits = "111";

    return dest_bits;
}

//format: dest=comp or comp;jmp
string Parser::comp() {
    //determine the format, then get everything to right of = or everything to left of ;
    string comp;

    int equalsIndex = current_command.find("=");
    if(equalsIndex >= 0) { //dest=comp
        comp = current_command.substr(equalsIndex + 1, current_command.length() - (equalsIndex + 1));
    }
    else {//comp;jmp
        int semicolonIndex = current_command.find(";");
        comp = current_command.substr(0, semicolonIndex);
    }

    //string::compare returns 0 on an equal comparison
    string comp_bits; //comp_bits=

    //18 rows on the a=0 side of the table
    if(!comp.compare("0"))          comp_bits = "0101010";
    else if(!comp.compare("1"))     comp_bits = "0111111";
    else if(!comp.compare("-1"))    comp_bits = "0111010";
    else if(!comp.compare("D"))     comp_bits = "0001100";
    else if(!comp.compare("A"))     comp_bits = "0110000";
    else if(!comp.compare("!D"))    comp_bits = "0001101";
    else if(!comp.compare("!A"))    comp_bits = "0110001";
    else if(!comp.compare("-D"))    comp_bits = "0001111";
    else if(!comp.compare("-A"))    comp_bits = "0110011";
    else if(!comp.compare("D+1"))   comp_bits = "0011111";
    else if(!comp.compare("A+1"))   comp_bits = "0110111";
    else if(!comp.compare("D-1"))   comp_bits = "0001110";
    else if(!comp.compare("A-1"))   comp_bits = "0110010";
    else if(!comp.compare("D+A"))   comp_bits = "0000010";
    else if(!comp.compare("D-A"))   comp_bits = "0010011";
    else if(!comp.compare("A-D"))   comp_bits = "0000111";
    else if(!comp.compare("D&A"))   comp_bits = "0000000";
    else if(!comp.compare("D|A"))   comp_bits = "0010101";
    //10 rows on the a=1 side of the table
    else if(!comp.compare("M"))     comp_bits = "1110000";
    else if(!comp.compare("!M"))    comp_bits = "1110001";
    else if(!comp.compare("-M"))    comp_bits = "1110011";
    else if(!comp.compare("M+1"))   comp_bits = "1110111";
    else if(!comp.compare("M-1"))   comp_bits = "1110010";
    else if(!comp.compare("D+M"))   comp_bits = "1000010";
    else if(!comp.compare("D-M"))   comp_bits = "1010011";
    else if(!comp.compare("M-D"))   comp_bits = "1000111";
    else if(!comp.compare("D&M"))   comp_bits = "1000000";
    else if(!comp.compare("D|M"))   comp_bits = "1010101";

    return comp_bits;
}

//format: comp;jmp
string Parser::jump() {
    string jump;

    int semicolonIndex = current_command.find(";");
    if(semicolonIndex < 0) { //did not find ;, so jump bits are unused
        jump = "null";
    }
    else {
        jump = current_command.substr(semicolonIndex + 1, current_command.length() - (semicolonIndex + 1));
    }

    
    //string::compare returns 0 on an equal comparison
    string jump_bits;
    if(!jump.compare("null")) jump_bits = "000";
    else if(!jump.compare("JGT")) jump_bits = "001";
    else if(!jump.compare("JEQ")) jump_bits = "010";
    else if(!jump.compare("JGE")) jump_bits = "011";
    else if(!jump.compare("JLT")) jump_bits = "100";
    else if(!jump.compare("JNE")) jump_bits = "101";
    else if(!jump.compare("JLE")) jump_bits = "110";
    else if(!jump.compare("JMP")) jump_bits = "111";

    return jump_bits;
}

void Parser::printTranslatedCommands() {
    for(int i = 0; i < translated_commands.size(); i++) {
        cout << translated_commands[i] << endl;
    }
}
//returns 15 bit binary string
string Parser::toBinary(int n) {
    string bits = "";
    
    int current = n;
    while(current) {
        if(current % 2) {
            bits += "1";
        }
        else {
            bits += "0";
        }
        current /= 2;
    }
    //bits is currently holding the binary number, but backwards. we need to pad to be 15 bits on the left,
    //but it's still backwards, so we can keep appending before reversing
    int numZeros = 15 - bits.length();
    for(int i = 0; i < numZeros; i++) {
        bits += '0';
    }

    if(bits.length() != 15) {
        cerr << "added wrong number of zeros\n";
        exit(3);
    }

    //now just reverse bits
    reverse(bits.begin(), bits.end());

    return bits;
}

Parser::Parser(string input_file) {
    asm_fs.open(input_file);
    if(!asm_fs.good()) {
        cerr << "There was a problem opening the file, maybe it doesn't exist?\n";
    }
}

bool Parser::hasMoreCommands() {
    return current_index + 1 < stripped_commands.size();
}


Parser::COMMAND_TYPE Parser::commandType() {
    switch(current_command.at(0)) {
    case '@':
        return A_COMMAND;
    case '(':
        return L_COMMAND;
    default:
        return C_COMMAND;
    }
}

//assumes no symbols
void Parser::translateAcommand() {
    char dummyAt;
    int n;

    stringstream ss(current_command);
    ss >> dummyAt >> n;

    translated_commands.push_back("0" + toBinary(n));
}

//format: dest=comp;jmp
void Parser::translateCcommand() {
    //it's either
    //comp;jmp
    //or
    //dest=comp
    string dest_bits = dest();
    string comp_bits = comp();
    string jump_bits = jump();
    
    translated_commands.push_back("111" + comp_bits + dest_bits + jump_bits);
}

void Parser::parse() {
    stripInput(); //populates stripped_commands

    convertToSymbolFree();
    processSymbolFree();
    
}

/** Initialize with predefined symbols in section 6.2.3 */
void Parser::initialization() {
    //SP 0, LCL 1, ARG 2, THIS 3, THAT 4, R0-R15 0-15, SCREEN 16384, KBD 24576
    symbol_table["SP"]      = 0;
    symbol_table["LCL"]     = 1;
    symbol_table["ARG"]     = 2;
    symbol_table["THIS"]    = 3;
    symbol_table["THAT"]    = 4;
    symbol_table["SCREEN"]  = 16384;
    symbol_table["KBD"]     = 24576;
    for(int i = 0; i < 16; i ++) {
        string i_str = to_string(i);
        symbol_table["R" + i_str] = i;
    }   
}

void Parser::first_pass() {
    //reset current_index
    current_index = -1;
    
    int currentROM = 0;
    while(hasMoreCommands()) {
        advance();
        
        COMMAND_TYPE current_command_type = commandType();
        
        switch(current_command_type) {
        case A_COMMAND:
        case C_COMMAND:
            currentROM++;
            break;
        case L_COMMAND:
            //save the label but don't include the parentheses
            //always in the form (Xxx)
            string label_symbol = current_command.substr(1, current_command.length() - 2);
            //TODO: we are assuming correct input but maybe abort if the key was already there
            //operator[] is a create/update, so it doesn't tell you if something already existed
            //for that key or not
            symbol_table[label_symbol] = currentROM;
            break;
        }
    }
}

bool Parser::current_is_symbolic() {
    /** according to 4.2.6 under Constants and Symbols, "a user-defined symbol
     *  can be any sequence of letters, digits, underscore, dot, dollar sign, and colon
     *  that does not begin with a digit"
     *  since the predefined symbols all start with a letter, we don't have to differentiate
     *  their processing from user-defined symbols 
     *  so it is anything in the list above that isn't a digit */
    //skip the @ sign
    char c = current_command.at(1);
    //since we are assuming well-formed input, just negate isdigit(c)
    return !isdigit(c);

}

void Parser::second_pass() {
    //reset current_index
    current_index = -1;
    
    int currentRAM = 16;
    while(hasMoreCommands()) {
        advance();
        
        COMMAND_TYPE current_command_type = commandType();
        
        switch(current_command_type) {
        case A_COMMAND:
            if(current_is_symbolic()) {
                string symbol = current_command.substr(1, current_command.length() - 1);
                if(symbol_table.find(symbol) != symbol_table.end()) { //found key
                    //we could write just to the table since we're
                    //not gonna use current_command again before calling advance, but 
                    //updating it both in current_command and the table just to be safe
                    current_command = "@" + to_string(symbol_table[symbol]);
                    stripped_commands[current_index] = current_command;
                }
                else { //didn't find key
                    //create key/value pair and replace
                    symbol_table[symbol] = currentRAM++;
                    current_command = "@" + to_string(symbol_table[symbol]);
                    stripped_commands[current_index] = current_command;
                }
            }
            break;
        case C_COMMAND:
        case L_COMMAND:
            break;
        }
    }
}

void Parser::convertToSymbolFree() {
    initialization();
    first_pass();
    second_pass();
}

void Parser::processSymbolFree() {
    //reset current_index
    current_index = -1;
    while(hasMoreCommands()) {
        advance();
        
        COMMAND_TYPE current_command_type = commandType();
        
        switch(current_command_type) {
        case A_COMMAND:
            translateAcommand();
            break;
        case C_COMMAND:
            translateCcommand();
            break;
        case L_COMMAND:
            //symbol-less assembler simply ignores L_COMMANDs
            break;
        }
    }
}

void Parser::advance() {
    //mutate current_command and current_index
    current_command = stripped_commands[++current_index];
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        cerr << "Must provide a single argument to the assembler\n";
        exit(1);
    }

    string asm_prog_name = argv[1];
    
    if(!endsWith(asm_prog_name, ".asm")) {
        cerr << "Must provide a .asm file\n";
    }

    Parser parser(asm_prog_name);
    parser.parse();
    parser.printTranslatedCommands();
    
    
    return 0;
}