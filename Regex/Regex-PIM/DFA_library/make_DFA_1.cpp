#include "make_DFA.h"
#include <bits/stdc++.h>
using namespace std;

#define DEBUG 1

string meta_character_set = "\\^$.|[]()*+?{},"; // 이 문자들은 메타 문자들로 문자 그 자체를 의미할 때는 반드시 \를 앞에 붙여야 한다.

int leaf_node_amount;

void print_string(string str)
{
    for(int i = 0; i < str.length(); i++)
    {
        if(str[i] > 0) cout << str[i];
        else cout << "\\" << (char) -str[i];
    }
    cout << endl;
}


class Node{
    public:
        char ch;
        bool is_nullable;
        bool is_leaf;
        int leaf_node_num;

        vector<int> firstpos;
        vector<int> lastpos;

        Node* right_child;
        Node* left_child;

        Node(char _ch, bool _is_nullable, bool _is_leaf, int _leaf_node_num)
        {
            ch = _ch;
            is_nullable = _is_nullable;
            is_leaf = _is_leaf;
            leaf_node_num = _leaf_node_num;
            if(_leaf_node_num)
            {
                firstpos.push_back(_leaf_node_num);
                lastpos.push_back(_leaf_node_num);
            }

            right_child = NULL;
            left_child = NULL;
        }
};

class State{
    public:
        int state_num;
        vector <int> nodes;

};

int     state_num;
int     char_set_size;
char    char_set_DPU[256];
int    char_hash[256];
int     is_any_char;
int**    DFA;
char    acceptable_states[256];




string hex_processing(string);

string iteration_processing(string);
    string square_braket_processing(string);
    string curly_braket_processing(string);

vector<char> escape_processing(string);
    int convert_hex_to_dec(char);

vector<char> make_augmented_regex(vector<char>&);

vector<char> make_postfix_regex(vector<char>&);

Node* make_expression_tree(vector<char>&);
    vector<int> vector_union(vector<int>&, vector<int>&);

vector<pair<int, char> > get_leaf_node_info(Node*);
    void _get_leaf_node_info(vector<pair<int, char> >&, Node*);

vector<vector<int> > make_followpos_table(Node*, int);
    void _make_followpos_table(vector<set<int> >&, Node*);

vector<char> get_leaf_node_letters(vector<pair<int, char> >&);
    

void make_DFA(char*);

void wrong_regex(string reason)
{
    std::cout << "WRONG_REGEX: " << reason << endl;
    exit(0);
}


int convert_hex_to_dec(char hex)
{
    if(hex >= '0' && hex <= '9') return hex - '0';
    else if(hex >= 'a' && hex <= 'z') return 10 + hex - 'a';
    else if(hex >= 'A' && hex <= 'Z') return 10 + hex - 'A';
    else
    {
        wrong_regex("잘못된 16진수 표현(convert_hex_to_dec)");
    }
}


string hex_processing(string regex)
{
    string result;

    for(int i = 0; i < regex.length(); i++)
    {
        if(regex[i] == '\\' && regex[i + 1] == 'x')
        {
            char hex_num[2];
            char hex_char;
            
            hex_num[0] = regex[i + 2];
            hex_num[1] = regex[i + 3];

            int hex2dec = 0;
            hex2dec = convert_hex_to_dec(hex_num[1]) + 16 * convert_hex_to_dec(hex_num[0]);

            if(hex2dec > 128)
            {
                wrong_regex("ASCII 범위를 넘어가는 16진수(escape_processing)");
            }

            hex_char = (char) hex2dec;

            if(meta_character_set.find(hex_char) != string::npos)
            {
                result += '\\';    
            }
            result += hex_char;
            i += 3;
        }
        else
        {
            result += regex[i];
        }
    }

#ifdef DEBUG
    cout << "hex_processed_regex: " << result << endl;
#endif

    return result;
}




string square_braket_processing(string regex)
{
    vector <string> splited_patterns;
    string processed_regex;

    int current_first = 0;

    // [] 안에 든 부분과 그렇지 않은 부분을 분리
    for(int i = 0; i < regex.length(); i++)
    {
        if(regex[i] == '[' && regex[i - 1] != '\\')
        {
            if(current_first < i)
            {
                splited_patterns.push_back(regex.substr(current_first, i - current_first));
            }
            current_first = i;
        }
        else if(regex[i] == ']' && regex[i - 1] != '\\')
        {
            splited_patterns.push_back(regex.substr(current_first, i - current_first + 1));
            current_first = i + 1;
        }
    }
    if(current_first != regex.length())
    {
        splited_patterns.push_back(regex.substr(current_first, regex.length() - current_first + 1));
    }


    int splited_pat_amount = splited_patterns.size();


    // []로 표현된 부분을 () 표현으로 변환
    for(int i = 0; i < splited_pat_amount; i++)
    {
        string current_pattern = splited_patterns[i];
        if(current_pattern[0] != '[') continue;


        int not_flag = 0;
        if(current_pattern[1] == '^')
        {
            not_flag = 1;
        }

        string inside_characters;
        for(int j = 1 + not_flag; j < current_pattern.size() - 1; j++)
        {
            if(current_pattern[j] == '-')
            {
                char front_ch = current_pattern[j - 1];
                char back_ch  = current_pattern[j + 1];

                if( ('0' <= front_ch && '9' >= front_ch && front_ch <= back_ch && '9' >= back_ch) || 
                    ('a' <= front_ch && 'z' >= front_ch && front_ch <= back_ch && 'z' >= back_ch) || 
                    ('A' <= front_ch && 'Z' >= front_ch && front_ch <= back_ch && 'Z' >= back_ch))
                {
                    for(char k = front_ch + 1; k < back_ch; k++)
                    {
                        inside_characters.append(1, k);
                    }
                }
                else
                {
                    inside_characters.append(1, '-');
                }
            }
            else
            {
                inside_characters.append(1, current_pattern[j]);
            }
        #ifdef DEBUG
            std::cout << inside_characters << endl;
        #endif
        }

        string converted_pat = "(";
        if(not_flag)
        {
            for(char ch = 32; ch < 127; ch++)
            {
                if(inside_characters.find(ch) == string::npos)
                {
                    if(meta_character_set.find(ch) != string::npos || ch =='#')
                        converted_pat.append(1, '\\');
                    converted_pat.append(1, ch);
                    converted_pat.append(1, '|');
                }
            #ifdef DEBUG
                std::cout << "converted_pat: " << converted_pat << endl;
            #endif
            }
        }
        else
        {
            int paren_on = 0;
            for(int in = 0; in < inside_characters.size(); in++)
            {
                if(inside_characters[in] == '(') paren_on = 1;
                if(inside_characters[in] == ')') paren_on = 0;
                converted_pat.append(1, inside_characters[in]);
                
                if(!paren_on) converted_pat.append(1, '|');
            }
        }

        converted_pat[converted_pat.size() - 1] = ')';
        splited_patterns[i] = converted_pat;
    }

    // 변환 완료된 regex를 반환
    for(int i = 0; i < splited_pat_amount; i++)
    {
        processed_regex += splited_patterns[i];
    }
#ifdef DEBUG
    std::cout << "square_braket_processing result: " << processed_regex << endl;
#endif
    return processed_regex;
}

string curly_braket_processing(string regex)
{
    vector <string> splited_patterns;
    string processed_regex;

    for(int i = 0; i < regex.length(); i++)
    {
        if(regex[i] == '{' && regex[i - 1] != '\\')
        {

            // {}로 감싸진 반복 횟수 추출
            int curly_start_pos = i;
            while(!(regex[i] == '}' && regex[i - 1] != '\\') && i < regex.length())
            {
                i++;
            }
            if(i >= regex.length()) wrong_regex("중괄호가 닫히지 않음(curly_barket_processing)");
            string range_str = regex.substr(curly_start_pos + 1, i - curly_start_pos - 1);
            // std::cout << "range_str: " << range_str << endl;
            

            // 최대, 최소 반복 횟수 파싱
            int min_cnt = 0;
            int max_cnt = 0;
            int commapos = range_str.find(',');
            if(commapos == string::npos)
            {
                min_cnt = stoi(range_str);
            }
            else if(commapos == 0)
            {
                wrong_regex("잘못된 수량자(curly_barket_processing)");
            }
            else if(commapos == range_str.length() - 1)
            {
                min_cnt = stoi(range_str.substr(0, range_str.length() - 1));
                max_cnt = -1;
            }
            else
            {
                min_cnt = stoi(range_str.substr(0, commapos));
                max_cnt = stoi(range_str.substr(commapos + 1, range_str.length() - commapos - 1));
            }

            if(min_cnt < 0 && max_cnt < -1)
            {
                wrong_regex("수량자 안에 음수가 들어갈 수 없음(curly_barket_processing)");
            }

            if(max_cnt < min_cnt && max_cnt > 0)
            {
                wrong_regex("최대 반복 횟수가 최소 반복 횟수보다 작을 수 없음(curly_barket_processing)");
            }
            // std::cout << "min_cnt: " << min_cnt << endl;
            // std::cout << "max_cnt: " << max_cnt << endl;


            //반복 대상 경정
            int proc_len = processed_regex.length();
            string iter_target;

            if(!proc_len) wrong_regex("가장 처음에 수량자가 등장할 수 없음(curly_barket_processing)");

            if(processed_regex[proc_len - 2] == '\\')
            {
                iter_target = processed_regex.substr(proc_len - 2, 2);
            }
            else if(processed_regex[proc_len - 1] != ')')
            {
                iter_target = processed_regex.substr(proc_len - 1, 1);
            }
            else
            {
                int paren_start_pos = proc_len - 1;

                while( !(processed_regex[paren_start_pos] == '(' && processed_regex[paren_start_pos - 1] != '\\') )
                {
                    paren_start_pos--;

                    if(paren_start_pos < 0) wrong_regex("괄호가 열리지 않음(curly_barket_processing)");
                }
                
                iter_target = processed_regex.substr(paren_start_pos, proc_len - paren_start_pos + 1);
            }
        #ifdef DEBUG
            std::cout << "iter_target: " << iter_target << endl;
        #endif

            //반복 실행
            for(int j = 1; j < min_cnt; j++)
            {
                #ifdef DEBUG
                    cout << "iter" << endl;
                #endif
                processed_regex += iter_target;
            }
            if(max_cnt == -1) processed_regex += "+";
            for(int j = 0; j < max_cnt - min_cnt; j++)
            {
                #ifdef DEBUG
                    cout << "iter" << endl;
                #endif
                processed_regex += iter_target + "?";
            }
        }
        else
        {
            processed_regex.append(1, regex[i]);
        }
    }
#ifdef DEBUG
    std::cout << "curly_braket_processing result: " << processed_regex << endl;
#endif

    return processed_regex;
}

string iteration_processing(string first_regex)
{
    string squ_brk_processed_regex = square_braket_processing(first_regex);
    string iterated_regex = curly_braket_processing(squ_brk_processed_regex);
#ifdef DEBUG
    std::cout << "iteration_processing result: " << iterated_regex << endl;
#endif
    return iterated_regex;
}

vector<char> escape_processing(string regex)
{
    vector<char> processed_regex;

    int regex_len = regex.length();


    for(int i = 0; i < regex_len; i++)
    {
        cout << "escape_processing: " << regex[i] << endl;
        if(regex[i] == '#')
        {
            processed_regex.push_back(-regex[i]);
        }
        else if(regex[i] == '\\')
        {
            char escape_target = regex[i + 1];


            if(meta_character_set.find(escape_target) != string::npos)
            {
                cout << "escape_target: " << escape_target << endl;
                processed_regex.push_back(-escape_target);

                
                if(escape_target == '?') cout << "Question mark" << endl;
                for(int aa = 0; aa < processed_regex.size(); aa++)
                {
                    cout << processed_regex[aa] << endl;
                }

                i++;
            }
            else if(escape_target == '\\')
            {
                processed_regex.push_back('\\');
                i++;
            }
            else if(escape_target == '/')
            {
                processed_regex.push_back('/');
                i++;
            }
            else if(escape_target == 's')
            {
                string space_set = "(\t|\n|\r|\f|\v)";
                int space_set_len = space_set.length();
                for(int j = 0; j < space_set_len; j++)
                {
                    processed_regex.push_back(space_set[j]);
                }
                i++;
            }
            else if(escape_target == 'd')
            {
                string num_set = "(0|1|2|3|4|5|6|7|8|9)";
                int num_set_len = num_set.length();
                for(int j = 0; j < num_set_len; j++)
                {
                    processed_regex.push_back(num_set[j]);
                }
                i++;
            }
            else if(escape_target == 'w')
            {
                string word_set = "(0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z|A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|_)";
                int word_set_len = word_set.length();
                for(int j = 0; j < word_set_len; j++)
                {
                    processed_regex.push_back(word_set[j]);
                }
                i++;
            }
            else if(escape_target == '.' || escape_target == '#' || escape_target == '?')
            {
                
                processed_regex.push_back(-escape_target);
                i++;
            }
            else
            {
                // wrong_regex("잘못된 백슬래쉬 사용(escape_processing)");

                processed_regex.push_back(escape_target);
                i++;
            }
        }
        else
        {
            processed_regex.push_back(regex[i]);
        }
    }

    // for(int i = 0; i < processed_regex.size(); i++)
    // {
    //     std::cout << (int) processed_regex[i] << endl;
    // }

    return processed_regex;
}

vector<char> make_augmented_regex(vector<char> &regex)
{
    vector<char> processed_regex;
    int regex_len = regex.size();

    processed_regex.push_back('(');

    for(int i = 0; i < regex_len - 1; i++)
    {
                                //0123456789ABC
        // meta_character_set : "\\^$.|[]()*+?{},"
        int c = meta_character_set.find(regex[i]);        // current character
        int n = meta_character_set.find(regex[i + 1]);    // next character

        if(c == string::npos) c = -1;
        if(n == string::npos) n = -1;

        if(
            (c == -1 && (n == -1 || n == 3)) || // LL
            (c ==  8 && (n == -1 || n == 3)) || // )L
            (c ==  9 && (n == -1 || n == 3)) || // *L
            (c ==  11 && (n == -1 || n == 3)) || // ?L
            (c ==  10 && (n == -1 || n == 3)) || // +L
            ((c == -1 || c == 3) && n ==  7) || // L(
            (c ==  8 && n ==  7) || // )(
            (c ==  9 && n ==  7) || // *(
            (c ==  11 && n ==  7) || // ?(
            (c ==  10 && n ==  7)    // +(
        )
        {
            processed_regex.push_back(regex[i]);
            processed_regex.push_back(',');
        }
        else
        {
            processed_regex.push_back(regex[i]);
        }
    }
    processed_regex.push_back(regex[regex_len - 1]);
    processed_regex.push_back(')');
    processed_regex.push_back(',');
    processed_regex.push_back('#');
#ifdef DEBUG
    std::cout << "augmented_regex: ";
    for(int i = 0; i < processed_regex.size(); i++)
    {
        if (processed_regex[i] < 0) std::cout << '\\' << (char) -processed_regex[i];
        else std::cout << processed_regex[i];
    }
    std::cout << endl;
#endif
    return processed_regex;
}

vector<char> make_postfix_regex(vector<char> &regex)
{
    vector<char> processed_regex;
    stack<char> S;
    int regex_len = regex.size();

    for(int i = 0; i < regex_len; i++)
    {
        char now = regex[i];

    #ifdef DEBUG
        cout << "now: " << now << endl;
    #endif
        if(meta_character_set.find(now) == string::npos || (now == '*' || now == '?' || now == '+'))
        {
            processed_regex.push_back(regex[i]);
        }
        else if(now == '.')
        {
            processed_regex.push_back(regex[i]);
        }
        else if(now == '\\')
        {
            processed_regex.push_back(-regex[++i]);
        }
        else if(now == '(')
        {
            S.push(now);
        }
        else if(now == ')')
        {
            while(!S.empty())
            {
                if(S.top() == '(')
                {
                    S.pop();
                    break;
                }
                else
                {
                    processed_regex.push_back(S.top());
                    S.pop();
                }
            }
        }
        else if(now == ',')
        {
            if(S.empty()) S.push(now);
            else if(S.top() == '|' || S.top() == '(') S.push(now);
            else if(S.top() == ',') processed_regex.push_back(now);
        }
        else if(now == '|')
        {
            if(S.empty()) S.push(now);
            else if(S.top() == '(') S.push(now);
            else if(S.top() == '|') processed_regex.push_back(now);
            else if(S.top() == ',')
            {
                while(!S.empty())
                {
                    if(S.top() == ',')
                    {
                        processed_regex.push_back(S.top());
                        S.pop();
                    }
                    else break;
                }

                if(S.empty()) S.push(now);
                else if(S.top() == '(')  S.push(now);
                else if(S.top() == '|') processed_regex.push_back(now);
            }
        }
    }
    while(!S.empty())
    {
        processed_regex.push_back(S.top());
        S.pop();
    }

#ifdef DEBUG
    std::cout << "postfix_regex: ";
    for(int i = 0; i < processed_regex.size(); i++)
    {
        if (processed_regex[i] < 0) std::cout << '\\' << (char) -processed_regex[i];
        else std::cout << processed_regex[i];
    }
    std::cout << endl;
#endif
    return processed_regex;
}

vector<int> vector_union(vector<int>& vec_1, vector<int>& vec_2)
{
    if(!is_sorted(vec_1.begin(), vec_1.end()))
    {
        sort(vec_1.begin(), vec_1.end());
    }

    if(!is_sorted(vec_2.begin(), vec_2.end()))
    {
        sort(vec_2.begin(), vec_2.end());
    }

    vector<int> res;
    vector<int>::iterator first1 = vec_1.begin(), first2 = vec_2.begin(), last1 = vec_1.end(), last2 = vec_2.end();

    while(!(first1 == last1 && first2 == last2))
    {
        if (first1 == last1) res.push_back(*first2++);
        else if (first2 == last2) res.push_back(*first1++);

        else if(*first1 < *first2) res.push_back(*first1++);
        else if(*first1 > *first2) res.push_back(*first2++);
        else
        {
            res.push_back(*first1++);
            first2++;
        }
    }

    return res;
}

void tree_test(Node* root)
{
    if(root->left_child != NULL) tree_test(root->left_child);
    if(root->right_child != NULL) tree_test(root->right_child);
    
#ifdef DEBUG
    if(root->ch < 0) std::cout << '\\' << (char) -root->ch;
    else std::cout << root->ch;
    cout << " " << root->leaf_node_num << endl;

    cout << "first_pos: ";
    for(int i = 0; i < root->firstpos.size(); i++)
    {
        cout << root->firstpos[i] << " ";
    }
    cout << endl;

    
    cout << "last_pos: ";
    for(int i = 0; i < root->lastpos.size(); i++)
    {
        cout << root->lastpos[i] << " ";
    }
    cout << endl;
#endif
}

Node* make_expression_tree(vector<char>& regex)
{
    int regex_len = regex.size();
    int last_leaf_node_num = 0;
    stack<Node*> node_stack;

    string operator_set = "*?+|,";

    for(int i = 0; i < regex_len; i++)
    {
        char ch = regex[i];

    #ifdef DEBUG
        std::cout << ch << endl;
    #endif
        if(operator_set.find(ch) == string::npos)
        {
            Node* new_node = new Node(ch, false, true, ++last_leaf_node_num);
            node_stack.push(new_node);
        }
        else if(ch == '|')
        {
            Node* right_child = node_stack.top();
            node_stack.pop();
            
            Node* left_child = node_stack.top();
            node_stack.pop();

            Node* new_node = new Node(ch, (left_child->is_nullable || right_child->is_nullable), false, 0);
            new_node->left_child = left_child;
            new_node->right_child = right_child;

            new_node->firstpos = vector_union(left_child->firstpos, right_child->firstpos);
            new_node->lastpos = vector_union(left_child->lastpos, right_child->lastpos);

            node_stack.push(new_node);
        }
        else if(ch == ',')
        {
            Node* right_child = node_stack.top();
            node_stack.pop();
            
            Node* left_child = node_stack.top();
            node_stack.pop();

            Node* new_node = new Node(ch, (left_child->is_nullable && right_child->is_nullable), false, 0);
            new_node->left_child = left_child;
            new_node->right_child = right_child;

            // firstpos
            if(left_child->is_nullable)
            {
                new_node->firstpos = vector_union(left_child->firstpos, right_child->firstpos);
            }
            else
            {
                new_node->firstpos = left_child->firstpos;
            }

            // lastpos
            if(right_child->is_nullable)
            {
                new_node->lastpos = vector_union(left_child->lastpos, right_child->lastpos);
            }
            else
            {
                new_node->lastpos = right_child->lastpos;
            }

            node_stack.push(new_node);
        }
        else if(ch == '*' || ch == '?')
        {
            Node* left_child = node_stack.top();
            node_stack.pop();

            Node* new_node = new Node(ch, true, false, 0);
            new_node->left_child = left_child;

            new_node->firstpos = left_child->firstpos;
            new_node->lastpos = left_child->lastpos;

            node_stack.push(new_node);
        }
        else if(ch == '+')
        {
            Node* left_child = node_stack.top();
            node_stack.pop();

            Node* new_node = new Node(ch, left_child->is_nullable, false, 0);
            new_node->left_child = left_child;

            new_node->firstpos = left_child->firstpos;
            new_node->lastpos = left_child->lastpos;

            node_stack.push(new_node);
        }
    }

    leaf_node_amount = last_leaf_node_num;

    // std::cout << leaf_node_amount << endl;

#ifdef DEBUG
    std::cout << "tree_test: ";
    tree_test(node_stack.top());
    std::cout << endl;
#endif
    return node_stack.top();
}

void _get_leaf_node_info(vector<pair<int, char> >& leaf_node_info, Node* node)
{
    if(node)
    {
        if(node->is_leaf)
        {
            leaf_node_info.push_back({node->leaf_node_num, node->ch});
        }
        else
        {
            _get_leaf_node_info(leaf_node_info, node->left_child);
            _get_leaf_node_info(leaf_node_info, node->right_child);
        }
    }
}

vector<pair<int, char> > get_leaf_node_info(Node* node)
{
    vector<pair<int, char> > leaf_node_info;
    _get_leaf_node_info(leaf_node_info, node);

    sort(leaf_node_info.begin(), leaf_node_info.end());
    
    // debugging
#ifdef DEBUG
    std::cout << "leaf_node_info" << endl;
    for(int i = 0; i < leaf_node_info.size(); i++)
    {
        std::cout << leaf_node_info[i].first << ": " << leaf_node_info[i].second << endl;
    }
#endif
    return leaf_node_info;
}

vector<char> get_leaf_node_letters(vector<pair<int, char> >& leaf_nodes)
{
    vector<char> leaf_node_letters;
    int leaf_nodes_amount = leaf_nodes.size();

    for(int i = 0; i < leaf_node_amount; i++)
    {
        leaf_node_letters.push_back(leaf_nodes[i].second);
    }

    //debugging
#ifdef DEBUG
    std::cout << "leaf_node_letters: ";
    for(int i = 0; i < leaf_node_letters.size(); i++)
    {
        char lnl = leaf_node_letters[i];
        if(lnl < 0) std::cout << '\\' << (char) -lnl;
        else std::cout << lnl;
    }
    std::cout << endl;
#endif
    return leaf_node_letters;
}

void _make_followpos_table(vector<set<int> >& followpos_table, Node* node)
{
    if(node)
    {
        char ch = node->ch;

        if(ch == '*' || ch == '+')
        {
            vector<int> firstpos = node->firstpos;
            vector<int> lastpos = node->lastpos;

            int firstpos_num = firstpos.size();
            int lastpos_num = lastpos.size();

            for(int i = 0; i < lastpos_num; i++)
            {
                int node_num = lastpos[i];
                for(int j = 0; j < firstpos_num; j++)
                {
                    followpos_table[node_num].insert(firstpos[j]);
                }
            }

            _make_followpos_table(followpos_table, node->left_child);
        }

        else if(ch == ',')
        {
            vector<int> firstpos = node->right_child->firstpos;
            vector<int> lastpos = node->left_child->lastpos;

            int firstpos_num = firstpos.size();
            int lastpos_num = lastpos.size();

            for(int i = 0; i < lastpos_num; i++)
            {
                int node_num = lastpos[i];
                for(int j = 0; j < firstpos_num; j++)
                {
                    followpos_table[node_num].insert(firstpos[j]);
                }
            }

            _make_followpos_table(followpos_table, node->left_child);
            _make_followpos_table(followpos_table, node->right_child);
        }

        else
        {
            _make_followpos_table(followpos_table, node->left_child);
            _make_followpos_table(followpos_table, node->right_child);
        }
    }
}

vector<vector<int> > make_followpos_table(Node* node, int followpos_table_size)
{
    vector<set<int> > temp_followpos_table(followpos_table_size + 1);
    _make_followpos_table(temp_followpos_table, node);

    vector<vector<int> > followpos_table(followpos_table_size + 1);
    for(int i = 1; i < followpos_table_size + 1; i++)
    {
        for(auto iter = temp_followpos_table[i].begin(); iter != temp_followpos_table[i].end(); iter++)
        {
            followpos_table[i].push_back(*iter);
        }
    }

    // debugging
#ifdef DEBUG

    cout << "followpos_table" << endl;
    for(int i = 1; i < followpos_table_size + 1; i++)
    {
        std::cout << i << ": ";
        for(int j = 0; j < followpos_table[i].size(); j++)
        {
            std::cout << followpos_table[i][j] << ' ';
        }
        std::cout << endl;
    }
#endif
    return followpos_table;
}


extern "C" {

void make_DFA(char* pattern){
    string first_regex;
    first_regex = pattern;
    
    // -1. hex processing

    string hex_processed_regex = hex_processing(first_regex);

    // 0. escape processing
    vector<char> escape_processed_regex_vec = escape_processing(hex_processed_regex);

    string escape_processed_regex;

    for(int i = 0; i < escape_processed_regex_vec.size(); i++)
    {
        escape_processed_regex += escape_processed_regex_vec[i];
    }

    // 1. iteration processing
    string iteration_processed_regex = iteration_processing(escape_processed_regex);

    vector<char> iteration_processed_regex_vec;

    for(int i = 0; i < iteration_processed_regex.length(); i++)
    {
        iteration_processed_regex_vec.push_back(iteration_processed_regex[i]);
    }

    // 2. escape processed regex를 augmented regex로 변환 (concate -> ,로 변환)
    vector<char> augmented_regex = make_augmented_regex(iteration_processed_regex_vec);

    // 3. augmented regex를 postfix 표현으로 변환
    vector<char> postfix_regex = make_postfix_regex(augmented_regex);

    // 4. postfix regex를 토대로 expression tree build
    Node* expression_tree = make_expression_tree(postfix_regex);

    // 5. tree를 순회하며 leaf node일 경우 번호와 문자를 저장
    vector<pair<int, char> > leaf_node_info = get_leaf_node_info(expression_tree);

    // 6. leaf node에 포함된 문자만 순서대로 저장
    vector<char> leaf_node_letters = get_leaf_node_letters(leaf_node_info);

#ifdef DEBUG
    std::cout << "leaf_node_letters: " << leaf_node_letters.size() << endl;
#endif
    // 7. followpos table build
    vector<vector <int> > followpos_table = make_followpos_table(expression_tree, leaf_node_letters.size());


    // 8. char_set string build
    string char_set;
    for(int i = 0; i < leaf_node_letters.size(); i++)
    {
        char processed_leaf_node_letters = leaf_node_letters[i];

        if(char_set.find(processed_leaf_node_letters) == string::npos) char_set.append(1, processed_leaf_node_letters);
    }
    if(char_set.find('.') != string::npos)
    {
        int temp_dot_pos = char_set.find('.');
        char_set[temp_dot_pos] = char_set[char_set.size() - 2];
        char_set[char_set.size() - 2] = '.';
    }

#ifdef DEBUG
    print_string(char_set);
#endif
    // 9. char hash table build
    for(int i = 0; i < 256; i++) char_hash[i] = -1;
    for(int i = 0; i < char_set.length(); i++)
    {
        if(char_set[i] == '.') char_hash[127] = i;
        else{
            int char_ascii = (char_set[i] > 0) ? char_set[i] : -char_set[i];
            char_hash[char_ascii] = i;
        }
    }
    
    // 10. transition table build
    leaf_node_letters.insert(leaf_node_letters.begin(), '$');

    vector <vector<int> > state_set(1); // state는 1번부터 시작, o번 state는 trap state
    vector <vector<int> > transition_table(1, vector <int>(char_set.length() - 1, 0));

    state_set.push_back(expression_tree->firstpos); // 첫 state는 루트 노드의 first_pos

#ifdef DEBUG
    cout << "root first_pos" << endl;

    for(int i = 0; i < state_set[1].size(); i++)
    {
        cout << state_set[1][i] << " ";
    }
    cout << endl;
#endif

    for(int state_num = 1; state_num < state_set.size(); state_num++)
    {
        vector <int> current_state = state_set[state_num];
        vector <int> transition;

#ifdef DEBUG
        cout << "state_num: " << state_num << endl;
#endif
        // char_set에 있는 문자를 순회
        // current_state에 존재하는 리프 노드 번호에 해당하는 문자가 current char이거나 .이면 다음 state 벡터 에 해당 리프 노드의 follow pos 저장
        for(int char_index = 0; char_index < char_set.size() - 1; char_index++)
        {        
            char char_in_char_set = char_set[char_index];

        #ifdef DEBUG
            cout << "char: " << char_in_char_set << endl;
        #endif
            
            vector <int> new_state;
            for(int leaf_node_index = 0; leaf_node_index < current_state.size(); leaf_node_index++)
            {
                int leaf_node_num = current_state[leaf_node_index];
                char leaf_node_char = leaf_node_letters[leaf_node_num];

                if(char_in_char_set == leaf_node_char || leaf_node_char == '.')
                {
                    new_state = vector_union(new_state, followpos_table[leaf_node_num]);
                }
            }

            int new_state_num = 0;
            for(int i = 0; i < state_set.size(); i++)
            {
                if(state_set[i] == new_state)
                {
                    new_state_num = i;
                    break;
                }
                if(i == state_set.size() - 1)
                {
                    state_set.push_back(new_state);
                    new_state_num = i + 1;
                    break;
                }
            }

            #ifdef DEBUG
            cout << "new_state" << endl;
            for(int i = 0; i < new_state.size(); i++)
            {
                cout << new_state[i] << " ";
            }
            cout << endl;
            #endif

            transition.push_back(new_state_num);

            #ifdef DEBUG
            cout << "transition" << endl;
            for(int i = 0; i < transition.size(); i++)
            {
                cout << transition[i] << " ";
            }
            cout << endl;
            #endif
        }
        transition_table.push_back(transition);
    }

#ifdef DEBUG
    std::cout << "transition_table" << endl;
    std::cout << "   ";
    for(int j = 0; j < char_set.length() - 1; j++)
    {
        std::cout << char_set[j] << " ";
    }
    std::cout << endl;
    for(int i = 0; i < transition_table.size(); i++)
    {
        std::cout << i << ": ";
        for(int j = 0; j < transition_table[i].size(); j++)
        {
            std::cout << transition_table[i][j] << ' ';
        }
        std::cout << endl;
    }
#endif

    // 12. DPU로 전송할 데이터 가공
    state_num = transition_table.size() - 1;
    char_set_size = char_set.length() - 1;

    if(char_set_size * state_num > 32 * 1024)
    {
        std::cout << "DFA의 크기가 너무 큽니다.\n" << endl;
        return;
    }

    for(int i = 0; i < char_set_size; i++)
    {
        char_set_DPU[i] = char_set[i];
    }

    int acceptable_node = leaf_node_letters.size() - 1;
#ifdef DEBUG
    std::cout << "acceptable_node: " << acceptable_node << endl;
#endif
    for(int i = 1; i < state_set.size(); i++)
    {
        for(int j = state_set[i].size() - 1; j >= 0; j--)
        {
            if(state_set[i][j] == acceptable_node)
            {
                acceptable_states[i] = 1;
            }
        }
    }

    //DFA 배열에 옮겨담기
    DFA = (int**) malloc((state_num + 1) * sizeof(int*));
    for(int i = 0; i <= state_num; i++)
    {
        DFA[i] = (int*) malloc(char_set_size * sizeof(int));
    }

    for(int i = 0; i <= state_num; i++)
    {
        for(int j = 0; j < char_set_size; j++)
        {
            DFA[i][j] = transition_table[i][j];
        }
    }

#ifdef DEBUG
    std::cout << "state_num," << state_num << endl;
    std::cout << "char_set_size," << char_set_size << endl;
    std::cout << "is_any_char," << is_any_char << endl;
    std::cout << "char_hash," << endl;
    for(int i = 0; i < 127; i++)
    {
        std::cout << (int)char_hash[i] << ',';
    }
    std::cout << (int)char_hash[127] << endl;
    std::cout << "acceptable_states" << endl;
    for(int i = 0; i <= state_num; i++)
    {
        std::cout << (int)acceptable_states[i];
    }
    std::cout << "transition_table" << endl;
    for(int i = 0; i <= state_num; i++)
    {
        for(int j = 0; j < char_set_size - 1; j++)
        {
            cout << DFA[i][j] << ',';
        }
        cout << DFA[i][char_set_size - 1] << endl;
    }
#endif
}


}