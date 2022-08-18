#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>
#include <fstream>
#include <sstream>

using namespace std;

vector<string> vm_files;
vector<vector<string> > current_stripped_input;
vector<string> hack_output;
ifstream current_ifs;
int label_counter = 0;

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
    //temp segment is R5-R12, so just use R5 and R6
    if(command == "add") {
        hack_output.push_back("@SP");   
        hack_output.push_back("D=M");   
        hack_output.push_back("D=D-1"); 
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        hack_output.push_back("@R5");   
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("D=M");
        hack_output.push_back("D=D-1");
        hack_output.push_back("D=D-1");
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        hack_output.push_back("@R5");
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

        hack_output.push_back("@R5"); 
        hack_output.push_back("M=D");

        hack_output.push_back("@SP");
        hack_output.push_back("D=M");
        hack_output.push_back("D=D-1");
        hack_output.push_back("D=D-1");
        hack_output.push_back("A=D");
        hack_output.push_back("D=M");

        hack_output.push_back("@R5");
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

void process_mem_access(const string& segment, const string& index) {
    if(segment == "constant") {
        hack_output.push_back("//START push " + segment + " " + index);
        hack_output.push_back("@" + index); //@index A gets index, M gets RAM[100] (irrelevant). index is int constant
        hack_output.push_back("D=A");          //D=A D gets index
        hack_output.push_back("@SP"); //A gets 0, M gets 256 (or current SP)
        hack_output.push_back("A=M"); //A=M A gets 256, M gets RAM[256]
        hack_output.push_back("M=D");  //RAM[256] gets index
        //increment stack pointer
        hack_output.push_back("@SP"); //A gets 0, M gets 256
        hack_output.push_back("M=M+1"); //RAM[256] gets incremented
        hack_output.push_back("//END push " + segment + " " + index);
    }
}

void process_program_flow() {

}

void process_function_calling() {

}

void write_bootstrap_code() {
    //TODO in chapter 8, write this code to initialize SP, among other things
}

void translate_stripped_input() {
    write_bootstrap_code();
    for(int i = 0; i < current_stripped_input.size(); i++) {
        const string& command = current_stripped_input[i][0];
        
        //arithmetic commands
        if((command == "add") || (command == "sub") || (command == "neg") ||
           (command == "eq")  || (command == "gt")  || (command == "lt")  ||
           (command == "and") || (command == "or")  || (command == "not")) {
            process_arith(command);
        }
        else if((command == "push") || (command == "pop")) {
            process_mem_access(current_stripped_input[i][1], current_stripped_input[i][2]);
        }
        else if((command == "label") || (command == "goto") || (command == "if-goto")) {
            //TODO: NOT SUPPORTED YET
            process_program_flow();
        }
        else if((command == "function") || (command == "call") || (command == "return")) {
            //TODO: NOT SUPPORTED YET
            process_function_calling();
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