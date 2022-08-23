#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <map>

using namespace std;

vector<string> vm_files;
vector<vector<string> > current_stripped_input;
vector<string> hack_output;
ifstream current_ifs;
int label_counter = 0;
string current_vm_function = "null";

map<string, string> seg_to_symbol = {{"local", "LCL"},
                                     {"argument", "ARG"},
                                     {"this", "THIS"},
                                     {"that", "THAT"},
                                     {"temp", "5"}, //temp starts at R5
                                     {"pointer", "3"}, //pointer is R3-R4
                                     {"static", "16"}
                                    };

/**Assume that inline comments are allowed, but they will be 
 * separated out by a space. For example, 
 * push constant 1 //someComment
 * is allowed but 
 * push constant 1//someComment
 * is now allowed */
void strip_input(string file) {
    current_stripped_input.clear();
    current_ifs.open(file);
    string line;
    while(getline(current_ifs, line)) {
        vector<string> words;
        //empty line
        if(line.length() == 1) {
            continue;
        }
        stringstream ss(line);
        string cleaned_line;
        string current_word;
        ss >> current_word;

        bool broke = false;
        while(!ss.eof()) {
            if(current_word.find("//") == 0) {
                broke = true;
                break;
            }
            words.push_back(current_word);
            ss >> current_word;
        }
        if(words.size()) {
            current_stripped_input.push_back(words);
        }
        
    }
    current_ifs.close();
}

void discover_vm_files(string arg) {
    DIR* dir;
    struct dirent* dir_entry;

    errno = 0;
    if((dir = opendir(arg.c_str())) == NULL) {
        switch(errno) {
        case EACCES: 
            printf("Permission denied\n"); 
            break;
        case ENOENT: 
            printf("Directory does not exist\n"); 
            break;
        case ENOTDIR:
            printf("%s is not a directory\n", arg.c_str()); 
            break;
        }
        exit(EXIT_FAILURE);
    }

    bool path_ends_in_slash = (*(arg.end() - 1) == '/');
    while((dir_entry = readdir(dir)) != nullptr) {
        string filename = dir_entry->d_name;
        string::iterator it = filename.end();
        bool is_vm_file = (
            (*(it - 3) == '.') &&
            (*(it - 2) == 'v') &&
            (*(it - 1) == 'm')
        );
        if(is_vm_file) {
            if(path_ends_in_slash) {
                vm_files.push_back(arg + filename);
            }
            else {
                vm_files.push_back(arg + "/" + filename);
            }
        }
    }    
}

bool valid_commandline_invocation(int argc, char** argv) {
    if(argc != 2) {
        cout << "Must provide a single argument representing either a directory or source file\n";
        return false;
    }

    //assume anything that doesn't end in .vm is a directory
    //otherwise it's a vm file
    bool is_dir = true;
    string arg = argv[1];

    //x.vm must be at least 4 characters, and has to end in .vm
    if(arg.size() >= 4) {
        string::iterator it = arg.end();
        is_dir = !(
            (*(it - 3) == '.') &&
            (*(it - 2) == 'v') &&
            (*(it - 1) == 'm')
        ); 
    }

    if(is_dir) {
        discover_vm_files(arg);
    }
    else {
        vm_files.push_back(arg);
    }
    
    
    return true;
}


void process_arith(const string& command) {
    //true temp segment is R13-R15
    if(command == "add") {
        hack_output.push_back("@SP");   
        hack_output.push_back("D=M");   
        hack_output.push_back("D=D-1"); 
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        hack_output.push_back("@R13");   
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("D=M");
        hack_output.push_back("D=D-1");
        hack_output.push_back("D=D-1");
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        hack_output.push_back("@R13");
        hack_output.push_back("D=D+M");

        hack_output.push_back("@SP");
        hack_output.push_back("A=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("A=A-1");
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }
    else if(command == "sub") {
        hack_output.push_back("@SP"); 
        hack_output.push_back("D=M"); 
        hack_output.push_back("D=D-1");
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        hack_output.push_back("@R13"); 
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("D=M");
        hack_output.push_back("D=D-1");
        hack_output.push_back("D=D-1");
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        hack_output.push_back("@R13");
        hack_output.push_back("D=D-M");

        hack_output.push_back("@SP");
        hack_output.push_back("A=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("A=A-1");
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }
    else if(command == "neg") {
        hack_output.push_back("@SP");
        hack_output.push_back("D=M");   
        hack_output.push_back("D=D-1"); 
        hack_output.push_back("A=D");
        hack_output.push_back("M=-M");

    }
    else if(command == "eq") {
        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("D=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("A=M");
        hack_output.push_back("D=D-A");
        //if D is 0, they are eq, so we want to return true (-1)
        //but we don't want to use a temp, and we want to take advantage of false
        //being hardcoded to be 0 and JEQ/JNE comparing against 0, so invert it
        //at the end

        //so we'll invert at the and (0 (000..00) inverts to -1(111...111))
        //if D is 0, we want to keep it at 0. Then the invert will make it -1 (true)
        //if D is not 0, we want to assign -1. Then the invert will make it 0 (false)


        string label = "D_is0." + to_string(label_counter++);
        hack_output.push_back("@" + label);
        hack_output.push_back("D;JEQ");

        hack_output.push_back("D=-1");

        hack_output.push_back("(" + label + ")");

        hack_output.push_back("D=!D"); //the invert we talked about above

        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("A=A-1");
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }
    else if(command == "gt") {
        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("D=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("D=M-D");

        hack_output.push_back("M=-1");
        //started off with x > y, => x - y > 0
        //if x - y > 0, we want to ultimately write -1, so initialize with
        //RAM[SP-1] with -1 and skip over the overwrite (M=0) if gt

        
        string label = "D_is0." + to_string(label_counter++);
        hack_output.push_back("@" + label);
        hack_output.push_back("D;JGT");
        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("A=A-1");
        hack_output.push_back("M=0");

        hack_output.push_back("(" + label + ")");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }
    else if(command == "lt") {
        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("D=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("D=M-D");

        hack_output.push_back("M=-1");
        //started off with x > y, => x - y > 0
        //if x - y > 0, we want to ultimately write -1, so initialize with
        //RAM[SP-1] with -1 and skip over the overwrite (M=0) if gt

        
        string label = "D_is0." + to_string(label_counter++);
        hack_output.push_back("@" + label);
        hack_output.push_back("D;JLT");
        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("A=A-1");
        hack_output.push_back("M=0");

        hack_output.push_back("(" + label + ")");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }
    else if(command == "and") {
        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("D=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("M=M&D");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }
    else if(command == "or") {
        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("D=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("M=M|D");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }
    else if(command == "not") {
        hack_output.push_back("@SP");
        hack_output.push_back("D=M");   
        hack_output.push_back("D=D-1"); 
        hack_output.push_back("A=D");
        hack_output.push_back("M=!M");
    }
    else {
        cout << "Unsupported arithmetic command\n";
        exit(1);
    }
}

/**push segment index: Push the value of segment[index] onto the stack
 * pop  segment index: Pop the top stack value and store it in segment[index].
 * */
void process_mem_access(const string& command, const string& segment, const string& index) {
    
    /**we can actually just expect the read/write address to be in D, then 
     * read/write on that address based on the value of command (push/pop). This way we don't
     * write duplicate source code for parallel push/pop subpaths
     * Note that constants will be provided as addresses, since it is an emulated virtual segment
     *  */
    if( (segment == "local") || 
        (segment == "argument") || 
        (segment == "this") || 
        (segment == "that") ||
        (segment == "temp") ||
        (segment == "pointer") ||
        (segment == "static") ) {
        hack_output.push_back("@" + seg_to_symbol.at(segment));
        if(segment == "temp" || segment == "pointer" || segment == "static") {
            hack_output.push_back("D=A");
        }
        else {
            hack_output.push_back("D=M");
        }
        hack_output.push_back("@" + index);
        hack_output.push_back("D=D+A");
    }
    //since constant is the only 'true' virtual segment, in that the vm translator provides
    //the values instead of it being read from somewhere in global ram, we do the handling here
    //and skip the 'reading' step of if(push) in the if(push)elseif(pop) below this set of ifs
    else if(segment == "constant") {
        hack_output.push_back("@" + index); 

        //special case code is written below this comment
        //normally we'd read from A, in the read step below, then push to stack
        //we can't read a constant from RAM, the implementation (this code, through `index`) provides it 
        hack_output.push_back("D=A");    
        hack_output.push_back("@SP"); 
        hack_output.push_back("A=M"); 
        hack_output.push_back("M=D");  
    }
    

    //for push, read address D and write that to the stack. Then increment SP
    if(command == "push") {
        //read step: constant was handled specially, so skip if segment == constant
        //read the value in D
        if(segment != "constant") {
            //copy D to A, then read into D. so essentially D = *D, but you can only deref A (with M)
            //Then write that to stack
            hack_output.push_back("A=D");
            hack_output.push_back("D=M");
            hack_output.push_back("@SP");
            hack_output.push_back("A=M");
            hack_output.push_back("M=D");
        }

        //increment step: increment SP
        hack_output.push_back("@SP"); //A gets 0, M gets 256
        hack_output.push_back("M=M+1"); //RAM[256] gets incremented
    }
    //for pop, pop stack and write to address in D. Then decrement SP
    else if(command == "pop") {
        //save D in a temp register
        hack_output.push_back("@R13");
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("A=M-1");
        hack_output.push_back("D=M");

        hack_output.push_back("@R13");
        hack_output.push_back("A=M");
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");
    }

}

/** label label: This command labels the current location in the function’s code.
 *   goto label: This command effects an unconditional goto operation, causing execution to continue from
 *      the location marked by the label. The jump destination must be located in the same function.
 *if-goto label: This command effects a conditional goto operation. The stack’s topmost value is popped;
 *      if the value is not zero, execution continues from the location marked by the label; otherwise, execution
 *      continues from the next command in the program. The jump destination must be located in the same
 *      function.
 * */
void process_program_flow(const string& command, const string& label) {
    const string full_label = current_vm_function + "$" + label;
    if(command == "label") {
        //page 194, figure 8.6 says that every label should be 
        //current_function + "$" + label
        hack_output.push_back("(" + full_label + ")");
    }
    else if(command == "goto") {
        hack_output.push_back("@" + full_label);
        hack_output.push_back("0;JMP");
    }
    else if(command == "if-goto") {
        hack_output.push_back("@SP");
        hack_output.push_back("A=M");
        hack_output.push_back("A=A-1");
        hack_output.push_back("D=M");
        hack_output.push_back("@SP");
        hack_output.push_back("M=M-1");

        hack_output.push_back("@" + full_label);
        hack_output.push_back("D;JNE");
    }
    else {
        cerr << "unsupported program flow command\n";
    }
}


/**function f n: Here starts the code of a function named f that has n local variables;
 *     call f m: Call function f, stating that m arguments have already been pushed onto the stack by the caller;
 *       return: Return to the calling function.
 * //Note that we pass the whole vector line because the vm instruction formats differ
 * */
void process_function_calling(const vector<string>& line) {
    const string& command = line.at(0);
    if(command == "function") {
        const string& f = line.at(1);
        const string& n = line.at(2);

        current_vm_function = f; 

        hack_output.push_back("(" + f + ")");
        //could do this in a .asm loop but just generate n 'push constant 0' instruction translations
        int n_int = atoi(n.c_str());
        
        hack_output.push_back("@SP");
        hack_output.push_back("A=M");
        for(int i = 0; i < n_int; i++) {
            hack_output.push_back("M=0");
            hack_output.push_back("A=A+1");
        }

        //increment stack pointer with an addition
        hack_output.push_back("@" + n);
        hack_output.push_back("D=A");

        hack_output.push_back("@SP");
        hack_output.push_back("M=M+D");
        
    }
    else if(command == "call") {
        const string& f = line.at(1);
        const string& m = line.at(2);

    }
    else if(command == "return") {
        //page 193, figure 8.5
        //use R13 for FRAME, use R14 for RET
        //(in the pseudocode, FRAME and RET are temps)
        //NOTE: the psuedocode is wrong as written. Need to add some implicit derefs
        //For example, FRAME=LCL isn't really useful unless you assume that you really mean
        //*FRAME = *LCL (since the symbol LCL is always 1)
        /**
         *FRAME=LCL
         *  RET= *(FRAME-5)
         * *ARG= pop()
         *   SP=ARG+1
         * THAT= *(FRAME-1)
         * THIS= *(FRAME-2)
         *  ARG= *(FRAME-3)
         *  LCL= *(FRAME-4)
         * goto RET
         * */
        hack_output.push_back("@LCL");
        hack_output.push_back("D=M");

        hack_output.push_back("@R13");
        hack_output.push_back("M=D"); //*FRAME=*LCL

        //D still has *LCL(==*FRAME, since we just assigned)
        hack_output.push_back("@5");
        hack_output.push_back("D=D-A");
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");
        hack_output.push_back("@R14");
        hack_output.push_back("M=D");

        //*ARG = pop()
        //put pop() into D
        hack_output.push_back("@SP");
        hack_output.push_back("D=M");
        hack_output.push_back("A=D-1");
        hack_output.push_back("D=M");

        // hack_output.push_back("@R15");
        // hack_output.push_back("M=D");
       
        //put *ARG into A
        hack_output.push_back("@ARG");
        hack_output.push_back("A=M");

        //put pop() (in D) into RAM[*ARG]
        hack_output.push_back("M=D");

        //SP=ARG+1
        hack_output.push_back("D=A+1");
        hack_output.push_back("@SP");
        hack_output.push_back("M=D");

        //THAT = *(FRAME-1)
        //Put FRAME-1 into A, then save *A(which is M) into D
        //We saved FRAME in R13
        hack_output.push_back("@R13");
        hack_output.push_back("D=M-1"); //FRAME-1
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        //put D into THAT
        hack_output.push_back("@THAT");
        hack_output.push_back("M=D");

        //THIS = *(FRAME-2)
        hack_output.push_back("@R13");
        hack_output.push_back("D=M-1"); //FRAME-1
        hack_output.push_back("D=D-1"); //-2
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        //put D into THIS
        hack_output.push_back("@THIS");
        hack_output.push_back("M=D");

        //ARG = *(FRAME-3)
        hack_output.push_back("@R13");
        hack_output.push_back("D=M-1"); //FRAME-1
        hack_output.push_back("D=D-1"); //-2
        hack_output.push_back("D=D-1"); //-3
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        //put D into ARG
        hack_output.push_back("@ARG");
        hack_output.push_back("M=D");

        //LCL = *(FRAME-4)
        hack_output.push_back("@R13");
        hack_output.push_back("D=M-1"); //FRAME-1
        hack_output.push_back("D=D-1"); //-2
        hack_output.push_back("D=D-1"); //-3
        hack_output.push_back("D=D-1"); //-4
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        //put D into LCL
        hack_output.push_back("@LCL");
        hack_output.push_back("M=D");

        //goto RET
        //we saved RET in R14, so get that to A and unconditionally jump
        hack_output.push_back("@R14");
        hack_output.push_back("A=M");
        hack_output.push_back("0;JMP");
        
    }
}

void write_bootstrap_code() {
    //TODO in chapter 8, write this code to initialize SP, among other things
}

void translate_stripped_input() {
    write_bootstrap_code();
    for(int i = 0; i < current_stripped_input.size(); i++) {
        //TODO delete this temp pushback
        string cur_line = "//BEGIN line [ ";
        for(int j = 0; j < current_stripped_input[i].size(); j++) {
            cur_line += current_stripped_input[i][j] + " ";
        }
        cur_line += "]";
        hack_output.push_back(cur_line);

        const string& command = current_stripped_input[i][0];
        
        //arithmetic commands
        if((command == "add") || (command == "sub") || (command == "neg") ||
           (command == "eq")  || (command == "gt")  || (command == "lt")  ||
           (command == "and") || (command == "or")  || (command == "not")) {
            process_arith(command);
        }
        else if((command == "push") || (command == "pop")) {
            process_mem_access(command, current_stripped_input[i][1], current_stripped_input[i][2]);
        }
        else if((command == "label") || (command == "goto") || (command == "if-goto")) {
            process_program_flow(command, current_stripped_input[i][1]);
        }
        else if((command == "function") || (command == "call") || (command == "return")) {
            process_function_calling(current_stripped_input[i]);
        }
        else {
            cout << "unrecognized command: " << command << endl;
            exit(1);
        }
    }
}

int main(int argc, char** argv) {
    if(!valid_commandline_invocation(argc, argv)) {
        exit(1);
    }
    

    //translate one file at a time
    for(int k = 0; k < vm_files.size(); k++) {
        strip_input(vm_files[k]);
        translate_stripped_input();    
    }

    for(int i = 0; i < hack_output.size(); i++) {
        cout << hack_output[i] << endl;
    }

}