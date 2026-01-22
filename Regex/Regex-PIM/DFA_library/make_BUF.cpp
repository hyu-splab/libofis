#include <bits/stdc++.h>
#include "make_DFA.h"

using namespace std;

int make_BUF(int* buffer, char* file_name)
{
    vector <int> input_buffer;
    ifstream DFA_info;
    int state_num, char_set_size;
    vector <int> index_mapping_table, acceptable_states_bitmap, transition_table;
    //DFA_info.open(file_name);
    DFA_info.open(file_name);

    if(DFA_info.is_open())
    {
        while(!DFA_info.eof())
        {
            string str;
            getline(DFA_info, str);

            if(str.empty()){
                continue;
            }

            string line_name, line_contents;
            int first_comma_pos = 0;
            for(int i = 0; ; i++)
            {
                if(str[i] == ',')
                {
                    first_comma_pos = i;
                    break;
                }
            }
            line_name = str.substr(0, first_comma_pos);
            line_contents = str.substr(first_comma_pos + 1);
            if(line_name == "state_num")
            {
                state_num = stoi(line_contents);
                // cout << state_num << endl;
            }
            else if(line_name == "char_set_size")
            {
                char_set_size = stoi(line_contents);
                // cout << char_set_size << endl;
            }
            else if(line_name == "char_hash")
            {
                int last_comma_pos = 0;
                int current_comma_pos = 0;
                for(int i = 0; i < line_contents.length(); i++)
                {
                    if(line_contents[i] == ',' || i == line_contents.length() - 1)
                    {
                        current_comma_pos = i;
                        if(i == line_contents.length() - 1) current_comma_pos++;
                        string num = line_contents.substr(last_comma_pos, current_comma_pos - last_comma_pos);
                        index_mapping_table.push_back(stoi(num));
                        last_comma_pos = current_comma_pos + 1;
                    }
                }
                // cout << index_mapping_table.size() << endl;
            }
            else if(line_name == "acceptable_states")
            {
                int last_comma_pos = 2;
                int current_comma_pos = 0;

                vector <int> temp_bits;

                for(int i = 2; i < line_contents.length(); i++)
                {
                    if(line_contents[i] == ',' || i == line_contents.length() - 1)
                    {
                        current_comma_pos = i;
                        if(i == line_contents.length() - 1) current_comma_pos++;
                        string num = line_contents.substr(last_comma_pos, current_comma_pos - last_comma_pos);
                        temp_bits.push_back(stoi(num));
                        last_comma_pos = current_comma_pos + 1;
                    }
                }

                for(int i = 0; i < temp_bits.size(); i += 32)
                {
                    int iter_amount = (((temp_bits.size() - i) > 32) ? 32 : (temp_bits.size() - i));

                    int bit_mask = 0;

                    for(int j = 0; j < iter_amount; j++)
                    {
                        bit_mask += temp_bits[i + j] << j;
                    }
                    acceptable_states_bitmap.push_back(bit_mask);
                }

                // cout << acceptable_states_bitmap.size() << endl;
            }
            else if(line_name == "transition_table")
            {
                int last_comma_pos = 0;
                int current_comma_pos = 0;
                for(int i = 0; i < line_contents.length(); i++)
                {
                    if(line_contents[i] == ',' || i == line_contents.length() - 1)
                    {
                        current_comma_pos = i;
                        if(i == line_contents.length() - 1) current_comma_pos++;
                        string num = line_contents.substr(last_comma_pos, current_comma_pos - last_comma_pos);
                        transition_table.push_back(stoi(num));
                        last_comma_pos = current_comma_pos + 1;
                    }
                }
                // cout << transition_table.size() << endl;
            }
        }
    }
    input_buffer.push_back(state_num);
    input_buffer.push_back(char_set_size);
    for(int i = 0; i < 128; i++)
    {
        input_buffer.push_back(index_mapping_table[i]);
    }

    for(int i = 0; i < acceptable_states_bitmap.size(); i++)
    {
        input_buffer.push_back(acceptable_states_bitmap[i]);
    }

    if(acceptable_states_bitmap.size() % 2)
    {
        input_buffer.push_back(0);
    }
    for(int i = char_set_size; i < transition_table.size(); i++)
    {
        input_buffer.push_back(transition_table[i]);
    }
    if(state_num * char_set_size % 2)
    {
        input_buffer.push_back(0);
    }
    // for(int i = 0; i < input_buffer.size(); i++)
    // {
    //     cout << input_buffer[i] << ' ';
    // }
    // cout << endl;
    // cout << input_buffer.size() << endl;
    
    // buffer allocation for C
    
    for(int i=0;i<input_buffer.size();i++){
        buffer[i] = input_buffer[i];
    }

    // for(int i = 0; i < input_buffer.size(); i++)
    // {
    //     cout << buffer[i] << ' ';
    // }
    // cout << endl;
    // cout << input_buffer.size() << endl;

    DFA_info.close();
    
    return input_buffer.size();
}