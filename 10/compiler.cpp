#include <iostream>
#include <dirent.h>
#include <string>
#include <vector>
#include <fstream>
#include <map>

using namespace std;

void discover_jack_files(string arg);
bool valid_commandline_invocation(int argc, char** argv);

vector<string> jack_files;

string current_jack_file;
ifstream current_ifs;

class lexer {

public:
    void tokenize();
    void print_tokens();

    enum STATE {
        ERROR = 0,
        NORMAL,
        INLINE_COMMENT,
        BLOCK_COMMENT,
        STRING_LIT
    };
    enum TOKEN {
        TOK_CLASS, //begin keywords
        TOK_CONSTRUCTOR,
        TOK_FUNCTION,
        TOK_METHOD,
        TOK_FIELD,
        TOK_STATIC,
        TOK_VAR,
        TOK_INT,
        TOK_CHAR,
        TOK_BOOLEAN,
        TOK_VOID,
        TOK_TRUE,
        TOK_FALSE,
        TOK_NULL,
        TOK_THIS,
        TOK_LET,
        TOK_DO,
        TOK_IF,
        TOK_ELSE,
        TOK_WHILE,
        TOK_RETURN,
        KW_END, //end keywords
        TOK_LBRACE,
        TOK_RBRACE,
        TOK_LPAREN,
        TOK_RPAREN,
        TOK_LBRACKET,
        TOK_RBRACKET,
        TOK_DOT,
        TOK_COMMA,
        TOK_SEMI,
        TOK_PLUS,
        TOK_MINUS,
        TOK_MULT,
        TOK_DIV,
        TOK_AND,
        TOK_OR,
        TOK_LT,
        TOK_GT,
        TOK_EQ,
        TOK_NOT,
        SYM_END, //end symbols
        TOK_INTCONST,
        TOK_STRCONST,
        TOK_ID
    };

    vector<pair<TOKEN, string> > tokens;
    STATE state;

    char *begin, *end; //pointers used to process begin/end of a word
    char* line_end; //compare to this instead of \n or \r because getline consumes those
    string current_line;

private:
    /** Positions begin/end, returning false if there was no next word */
    bool next_word();
    void handle_block();
    void handle_inline();
    void handle_string();

    pair<TOKEN, string> make_token(string token);

    bool is_symbol(char c);
    bool is_number(string s);

    bool first_next_word = true;

    string string_lit;

    int line_no = 0;
};

void lexer::print_tokens() {
    cout << "<tokens>" << endl;
    for(auto p : tokens) {
        string opentag, closetag; 
        string contents = "";
        if((p.first >= TOK_CLASS) && (p.first < KW_END) ) {
            opentag = "<keyword>";
            closetag = "</keyword>";
        }
        else if((p.first >= TOK_LBRACE) && (p.first < SYM_END) ) {
            opentag = "<symbol>";
            closetag = "</symbol>";
        }
        else if(p.first == TOK_INTCONST) {
            opentag = "<integerConstant>";
            closetag = "</integerConstant>";
        }
        else if(p.first == TOK_STRCONST) {
            opentag = "<stringConstant>";
            closetag = "</stringConstant>";
        }
        else if(p.first == TOK_ID) {
            opentag = "<identifier>";
            closetag = "</identifier>";
        }
        else {
            cout << "ERROR: UNKNOWN TOK TYPE\n";
            exit(2);
        }

        contents += " ";
        switch(p.first) {
        case TOK_CLASS: {
            contents += "class";
            break;
        }
        case TOK_CONSTRUCTOR: {
            contents += "constructor";
            break;
        }
        case TOK_FUNCTION: {
            contents += "function";
            break;
        }
        case TOK_METHOD: {
            contents += "method";
            break;
        }
        case TOK_FIELD: {
            contents += "field";
            break;
        }
        case TOK_STATIC: {
            contents += "static";
            break;
        }
        case TOK_VAR: {
            contents += "var";
            break;
        }
        case TOK_INT: {
            contents += "int";
            break;
        }
        case TOK_CHAR: {
            contents += "char";
            break;
        }
        case TOK_BOOLEAN: {
            contents += "boolean";
            break;
        }
        case TOK_VOID: {
            contents += "void";
            break;
        }
        case TOK_TRUE: {
            contents += "true";
            break;
        }
        case TOK_FALSE: {
            contents += "false";
            break;
        }
        case TOK_NULL: {
            contents += "null";
            break;
        }
        case TOK_THIS: {
            contents += "this";
            break;
        }
        case TOK_LET: {
            contents += "let";
            break;
        }
        case TOK_DO: {
            contents += "do";
            break;
        }
        case TOK_IF: {
            contents += "if";
            break;
        }
        case TOK_ELSE: {
            contents += "else";
            break;
        }
        case TOK_WHILE: {
            contents += "while";
            break;
        }
        case TOK_RETURN: {
            contents += "return";
            break;
        }
        case TOK_LBRACE: {
            contents += "{";
            break;
        }
        case TOK_RBRACE: {
            contents += "}";
            break;
        }
        case TOK_LPAREN: {
            contents += "(";
            break;
        }
        case TOK_RPAREN: {
            contents += ")";
            break;
        }
        case TOK_LBRACKET: {
            contents += "[";
            break;
        }
        case TOK_RBRACKET: {
            contents += "]";
            break;
        }
        case TOK_DOT: {
            contents += ".";
            break;
        }
        case TOK_COMMA: {
            contents += ",";
            break;
        }
        case TOK_SEMI: {
            contents += ";";
            break;
        }
        case TOK_PLUS: {
            contents += "+";
            break;
        }
        case TOK_MINUS: {
            contents += "-";
            break;
        }
        case TOK_MULT: {
            contents += "*";
            break;
        }
        case TOK_DIV: {
            contents += "/";
            break;
        }
        case TOK_AND: {
            contents += "&amp;";
            break;
        }
        case TOK_OR: {
            contents += "|";
            break;
        }
        case TOK_NOT: {
            contents += "~";
            break;
        }
        case TOK_LT: {
            contents += "&lt;";
            break;
        }
        case TOK_GT: {
            contents += "&gt;";
            break;
        }
        case TOK_EQ: {
            contents += "=";
            break;
        }
        case TOK_INTCONST:
        case TOK_STRCONST:
        case TOK_ID: {
            contents += p.second;
            break;
        }
        }
        contents += " ";
        
        cout << opentag + contents + closetag << endl;
    }
    cout << "</tokens>" << endl;
}

bool lexer::is_number(string s) {
    for(char c : s) {
        if(!isdigit(c)) {
            return false;
        }
    }

    return true;
}

bool lexer::is_symbol(char c) {
    return (
        (c == '{') ||
        (c == '}') ||
        (c == '(') ||
        (c == ')') ||
        (c == '[') ||
        (c == ']') ||
        (c == '.') ||
        (c == ',') ||
        (c == ';') ||
        (c == '+') ||
        (c == '-') ||
        (c == '*') ||
        (c == '/') ||
        (c == '&') ||
        (c == '|') ||
        (c == '<') ||
        (c == '>') || 
        (c == '=') || 
        (c == '~')
    );
}

pair<lexer::TOKEN, string> lexer::make_token(string token) {
    if(token == "class") {
        return make_pair(TOK_CLASS, "");
    }
    else if(token == "constructor") {
        return make_pair(TOK_CONSTRUCTOR, "");
    }
    else if(token == "function") {
        return make_pair(TOK_FUNCTION, "");
    }
    else if(token == "method") {
        return make_pair(TOK_METHOD, "");
    }
    else if(token == "field") {
        return make_pair(TOK_FIELD, "");
    }
    else if(token == "static") {
        return make_pair(TOK_STATIC, "");
    }
    else if(token == "var") {
        return make_pair(TOK_VAR, "");
    }
    else if(token == "int") {
        return make_pair(TOK_INT, "");
    }
    else if(token == "char") {
        return make_pair(TOK_CHAR, "");
    }
    else if(token == "boolean") {
        return make_pair(TOK_BOOLEAN, "");
    }
    else if(token == "void") {
        return make_pair(TOK_VOID, "");
    }
    else if(token == "true") {
        return make_pair(TOK_TRUE, "");
    }
    else if(token == "false") {
        return make_pair(TOK_FALSE, "");
    }
    else if(token == "null") {
        return make_pair(TOK_NULL, "");
    }
    else if(token == "this") {
        return make_pair(TOK_THIS, "");
    }
    else if(token == "let") {
        return make_pair(TOK_LET, "");
    }
    else if(token == "do") {
        return make_pair(TOK_DO, "");
    }
    else if(token == "if") {
        return make_pair(TOK_IF, "");
    }
    else if(token == "else") {
        return make_pair(TOK_ELSE, "");
    }
    else if(token == "while") {
        return make_pair(TOK_WHILE, "");
    }
    else if(token == "return") {
        return make_pair(TOK_RETURN, "");
    }
    else if(token == "{") {
        return make_pair(TOK_LBRACE, "");
    }
    else if(token == "}") {
        return make_pair(TOK_RBRACE, "");
    }
    else if(token == "(") {
        return make_pair(TOK_LPAREN, "");
    }
    else if(token == ")") {
        return make_pair(TOK_RPAREN, "");
    }
    else if(token == "[") {
        return make_pair(TOK_LBRACKET, "");
    }
    else if(token == "]") {
        return make_pair(TOK_RBRACKET, "");
    }
    else if(token == ".") {
        return make_pair(TOK_DOT, "");
    }
    else if(token == ",") {
        return make_pair(TOK_COMMA, "");
    }
    else if(token == ";") {
        return make_pair(TOK_SEMI, "");
    }
    else if(token == "+") {
        return make_pair(TOK_PLUS, "");
    }
    else if(token == "-") {
        return make_pair(TOK_MINUS, "");
    }
    else if(token == "*") {
        return make_pair(TOK_MULT, "");
    }
    else if(token == "/") {
        return make_pair(TOK_DIV, "");
    }
    else if(token == "&") {
        return make_pair(TOK_AND, "");
    }
    else if(token == "|") {
        return make_pair(TOK_OR, "");
    }
    else if(token == "~") {
        return make_pair(TOK_NOT, "");
    }
    else if(token == "<") {
        return make_pair(TOK_LT, "");
    }
    else if(token == ">") {
        return make_pair(TOK_GT, "");
    }
    else if(token == "=") {
        return make_pair(TOK_EQ, "");
    }
    else if(is_number(token)) {
        return make_pair(TOK_INTCONST, token);
    }
    else { //identifier
        return make_pair(TOK_ID, token);
    }
}

/** Precondition: *begin == '/' && *(begin+1) == '*'
 * 
 * */
void lexer::handle_block() {
    //call to next_word assumes end is positioned one after the current word
    end = begin + 1;
    //since this is a block comment, this is guaranteed to return true before eof
    //otherwise, user didn't close the comment
    if(!next_word()) {
        cout << "user didn't close comment\n";
        exit(1);
    }
    else {
        // process this word a character at a time, because it can contain the closing
        // "*/" anywhere
        bool reachedCloseComment = false;
        while(!reachedCloseComment) {
            //on encountering a new line, we don't want to increment begin
            //so keep the begin++ in an else/default branch
            if( (begin == line_end) || (begin == end)) {
                if(!next_word()) {
                    cout << "never closed block comment\n";
                    exit(2);
                }
            }
            else if( (*begin == '*') && (*(begin + 1) == '/') ) {
                //in order for the subsequent call to next_word() 
                //on the next iteration of caller's (lexer::tokenize) while loop,
                //we need end to point to one after the current word, and that
                //has to be white space. so make *(begin + 1) a whitespace char
                //then assign end = begin + 1 so that it looks like this is a separate
                //word to be discovered next.
                reachedCloseComment = true;
                end = begin + 1;
                *end = ' ';
            } 
            else {
                begin++;
            }
            
        }        
    }
}

/** Precondition: *begin == '/' && *(begin+1) == '/'
 * 
 * */
void lexer::handle_inline() {
    while( end != &*current_line.end()) {
        end++;
    }
    //note that the call to next_word on the next iteration of 
    //caller's loop will be setup to handle getting the next line

}

/** Precondition: *begin == '"'
 * 
 * */
void lexer::handle_string() {
    string str = "";
    begin++; //start after the quotation mark
    while(*begin != '"') {
        str.push_back(*begin);
        if( begin == line_end ) {
            cout << "user didn't close string\n";
            exit(2);
        }
        begin++;
    }
    tokens.push_back(make_pair(TOK_STRCONST, str));
    //in order for the subsequent call to next_word() 
    //on the next iteration of caller's (lexer::tokenize) while loop,
    //we need end to point to one after the closing quote
    //from the while loop above, begin points at something containing ",
    //so advance it once and assign to begin
    begin++;
    end = begin;
}

bool lexer::next_word() {
    if(current_ifs.eof()) {
        return false;
    }

    //first call to next_word
    if(first_next_word) {
        first_next_word = false;
        getline(current_ifs, current_line); line_no++;
        end = &*current_line.begin();
        line_end = &*current_line.end();
    }

    //find the first non-whitespace (excluding newlines)
    while( (end != line_end) && isspace(*end) ) {
        end++;
    }

    //reached end of line before finding next word, so call getline then recurse
    if(end == line_end) {
        getline(current_ifs, current_line); line_no++;
        end = &*current_line.begin();
        line_end = &*current_line.end();
        return next_word();
    }
    else { //found first non-whitespace, so save it in begin then keep advancing end
        begin = end;
        while( (end != line_end) && !isspace(*end)) {
            end++;
        }
        return true;
    }


    // if(state == INLINE_COMMENT) {
    //     return false;
    // }

    // if(state == BLOCK_COMMENT) {
    //     //in all other cases, we inserted a token and by doing so advanced begin
    //     //all the way up to end. so just do that directly then get the next word
    //     //as usual
        
    //     begin = end;
    // }

    // while(isspace(*begin)) {
    //     begin++;
    // }


    // if(*begin == '\0') {
    //     return false;
    // }

    //begin is at a non-whitespace character
    // end = begin;
    // while(!isspace(*end)) {
    //     end++;
    // }

    //end is one-past the last char of the word starting at begin
    // return true;

}

void lexer::tokenize() {
    current_ifs.open(current_jack_file);
    begin = nullptr, end = nullptr;


    //initial design was doing 
    //while(getline()) {
        // while(next_word()) {
            //do stuff
        // }
    // }
    //instead, try doing one based only on next_word() that calls
    //getline whenever it needs to. this way, only next_word() 
    //will call getline and maintaining begin/end becomes easier
    //outside of INLINE/STRING, we will process a word at a time. that means
    //we can have multiple tokens in one word, like in Memory.deAlloc(this); (7 tokens)
    //we handle BLOCK/INLINE/STRING immediately
    //any tokens should be added before next iteration and call to next_word
    while(next_word()) {
         if( (*begin == '/') && (*(begin + 1) == '*') ) {
            handle_block();
         }
         else if( (*begin == '/') && (*(begin + 1) == '/') ) {
            handle_inline();
         }
         else if( (*begin == '"') ) {
            handle_string();
         }
         else {
            string str = "";
            while( (begin != end) && !is_symbol(*begin) && (*begin != '"') ) {
                str.push_back(*begin);
                begin++;
                //since the beginning of a comment/string can start in the middle of a word,
                //we parse out tokens and replace the last one with a space so we
                //can call next_word again and get the expected handling for
                //encountering a comment/string
            }
            if(str != "") {
                tokens.push_back(make_token(str));
            }

            if(is_symbol(*begin)) {
                str = "";
                str.push_back(*begin);
                begin++;
                tokens.push_back(make_token(str));
            }
            
            end = begin;
            // *end = ' ';
         }
    }

    // while(getline(current_ifs, line)) {
    //     if(state == INLINE_COMMENT) {
    //         state = NORMAL;
    //     }

        
    //     if(!line.length()) { //empty line
    //         continue; 
    //     }

    //     //start at the first non-whitespace character
    //     begin = &line.at(0);

    //     while(next_word()) {            
    //         if(state == NORMAL) {
    //             //since a word can have multiple tokens, we need to delimit the current
    //             //word by checking for any symbol tokens (or '"' character)
    //             char* old_begin = begin;
    //             while(begin != end) {
    //                 if(*begin == '\0') {
    //                     break;
    //                 }

    //                 if(*begin == '\"') {
    //                     begin++;
    //                     string_lit = "";
    //                     state = STRING_LIT;
    //                     while(*begin != '\"') {
    //                         string_lit.push_back(*begin);
    //                         if(*begin == '\n') {
    //                             cerr << "ERROR: String cannot span multiple lines\n";
    //                             exit(1);
    //                         }
    //                         begin++;
    //                     }
    //                     tokens.push_back(make_pair(TOK_STRCONST, string_lit));
                        
    //                     end = ++begin;
    //                     old_begin = begin;

    //                     state = NORMAL;
                        
    //                     break;
    //                 }
    //                 //check for double // first otherwise is_symbol will pick up a division symbol
    //                 else if( (*begin == '/') && ((*(begin+1)) == '/') ) {
    //                     state = INLINE_COMMENT;
    //                     break;
    //                 }
    //                 //check for /* first otherwise is_symbol will pick up a division symbol
    //                 else if( (*begin == '/') && ((*(begin+1)) == '*') ) {
    //                     state = BLOCK_COMMENT;
    //                     //handle begin right now

                        
    //                     while(*begin != '\0') {
    //                         if((*begin == '*') && (*(begin+1)) == '/') {
    //                             state = NORMAL;
    //                             //we want to advance by 2 total so do 1 here and then another at the begin++ at the end of the outer while
    //                             begin = begin + 2; 
    //                             end = begin;
    //                             break;
    //                         }
    //                         begin++;
    //                     }
                        

    //                     old_begin = begin; //so we don't push a token back
    //                     if(*begin != '\0') {
    //                         continue;
    //                     }
    //                     else {
    //                         break;
    //                     }
                        
    //                 }
    //                 else if(is_symbol(*begin)) {
    //                     //symbols mark the end of the terminal being built up (if any),
    //                     //and also the current symbol is a terminal
    //                     if(old_begin != begin) {
    //                         //we were building something up before encountering this terminal
    //                         string temp = "";
    //                         while(old_begin != begin) {
    //                             temp.push_back(*old_begin);
    //                             old_begin++;
    //                         }
    //                         tokens.push_back(make_token(temp));
    //                     }

    //                     //push the terminal
    //                     string terminal = "";
    //                     terminal.push_back(*begin);
    //                     tokens.push_back(make_token(terminal));
    //                     old_begin = begin + 1;
    //                      //to avoid the while loop old_begin != begin 
    //                      //(begin++ two lines down)
    //                 }
    //                 begin++;
    //             }

    //             string token = "";
    //             while(old_begin != begin) {
    //                 token.push_back(*old_begin);
    //                 old_begin++;
    //             }
    //             if(token != "") {
    //                 tokens.push_back(make_token(token));
    //             }
                
    //         }
    //         else if(state == BLOCK_COMMENT) {
    //             if( (*(begin+1) == '/') && ((*begin) == '*') ) {
    //                 state = NORMAL;
    //                 begin = begin + 2;
    //                 end = begin;
    //             } 
    //         }
    //     }
    // }
}

class parser {

};


bool valid_commandline_invocation(int argc, char** argv) {
    if(argc != 2) {
        cout << "Must provide a single argument representing either a directory or source file\n";
        return false;
    }

    //assume anything that doesn't end in .vm is a directory
    //otherwise it's a vm file
    bool is_dir = true;
    string arg = argv[1];

    //x.jack must be at least 6 characters, and has to end in .jack
    if(arg.size() >= 6) {
        string::iterator it = arg.end();
        is_dir = !(
            (*(it - 5) == '.') &&
            (*(it - 4) == 'j') &&
            (*(it - 3) == 'a') &&
            (*(it - 2) == 'c') &&
            (*(it - 1) == 'k')
        ); 
    }

    if(is_dir) {
        discover_jack_files(arg);
    }
    else {
        jack_files.push_back(arg);
    }
    
    return true;
}

void discover_jack_files(string arg) {
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
        bool is_jack_file = (
            (*(it - 5) == '.') &&
            (*(it - 4) == 'j') &&
            (*(it - 3) == 'a') &&
            (*(it - 2) == 'c') &&
            (*(it - 1) == 'k')
        );
        if(is_jack_file) {
            if(path_ends_in_slash) {
                jack_files.push_back(arg + filename);
            }
            else {
                jack_files.push_back(arg + "/" + filename);
            }
        }
    }    
}


int main(int argc, char** argv) {
    if(!valid_commandline_invocation(argc, argv)) {
        exit(1);
    }

    lexer my_lexer;
    //compile one file at a time
    for(int k = 0; k < jack_files.size(); k++) {
        current_jack_file = jack_files[k];
        my_lexer.tokenize();
    }

    my_lexer.print_tokens();

}