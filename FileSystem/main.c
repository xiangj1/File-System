//
//  main.c
//  cs143proj1
//
//  Created by Xiang Jiang on 2017/10/22.
//  Copyright © 2017年 Xiang Jiang. All rights reserved.
//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DESCRIPTOR_SIZE 24
#define FILE_NAME_LENGTH 4



FILE* command_file;

unsigned int ldisk[64][16] = {0}; //64 blocks & each block contains 16 int
unsigned int bitmap[2] = {0}; //assume bitmap holds in ldisk[0]
char directory[DESCRIPTOR_SIZE-1][FILE_NAME_LENGTH] = {0}; //dir cost 1 descriptor
unsigned int MASK[32];
unsigned int MASK2[32];

typedef struct
{
    char r_w_buffer[64];
    int current_position;
    int index;
}Open_File_Table;
Open_File_Table OFT[4];

typedef struct
{
    int file_length;
    int block1;
    int block2;
    int block3;
}Descriptor;
Descriptor descriptor[DESCRIPTOR_SIZE];

int * d2f = &descriptor[2].file_length;

int open_command_file()
{
    char command_file_path[100] = "/Volumes/NO\ NAME/proj1/win/input01.txt";
//    printf("Enter the command file full path:");
//    scanf("%s", command_file_path);
//    printf("The file_path is %s\n", command_file_path);
    
    if((command_file = fopen(command_file_path, "r")) == NULL)
    {
        printf("Error\n");
        return 1;
    }
    return 0;
}

int read_block(int j, char *p)
{
    char* c;
    c = (char *)&ldisk[j];
    
    if(strlen(c) < 4)
    {
        for(int i = 0; i < strlen(c); i++)
            p[i] = c[i];
        return 0;
    }
    
    for(int i = 0; i < 64; i++)
        p[i] = c[i];
    
    
    return 0;
}

int write_block(int j, char *p)
{
    char* c;
    c = (char *)&ldisk[j];
    
    int size = (int)strlen(p);

    for(int i = 0; i < size; i++)
        c[i] = p[i];
    
    for(int i = size; i < 64; i++)
        c[i] = 0;
    return 0;
}

int initial_MASK()
{
    MASK[31] = 1;
    for(int i = 30; i >= 0; i--)
        MASK[i] = MASK[i+1] << 1;
    
    for(int i = 0; i < 32; i++)
        MASK2[i] = ~MASK[i];
    return 0;
}

int initial_OFT()
{
    for(int i = 0; i < DESCRIPTOR_SIZE; i++)
    {
        OFT[i].index = -1;
        OFT[i].current_position = 0;
        for(int j = 0; j < 64; j++)
            OFT[i].r_w_buffer[j] = 0;
    }
    OFT[0].index = 0;
    return 0;
}

int initial_bitmap()
{
    for(int i = 0; i < DESCRIPTOR_SIZE/4+1; i++)
        bitmap[0] = bitmap[0] | MASK[i]; //BM: 1000 0000 0000 0000 0000 0000 0000 0000
    
    return 0;
}

int initial_descripor()
{
    for(int i = 0; i < DESCRIPTOR_SIZE; i++)
    {
        descriptor[i].file_length = -1;
        descriptor[i].block1 = 0;
        descriptor[i].block2 = 0;
        descriptor[i].block3 = 0;
    }
    return 0;
}

int initial_directory()
{
    descriptor[0].file_length = 0; //first descriptor
    return 0;
}

int free_block(int i)
{
    if(i < 32) //if the block number is less then 32, then it will be stored in bitmap[0]
        bitmap[0] = bitmap[0] & MASK2[i];
    else
        bitmap[1] = bitmap[1] & MASK2[i];
    return 0;
}

void clear_buffer(int OFT_index)
{
    for(int i = 0; i < 64; i++)
        OFT[OFT_index].r_w_buffer[i] = 0;
}
void load_buffer(int OFT_index)
{
    if(descriptor[OFT[OFT_index].index].file_length <= 0)
        return;
    
    switch (OFT[OFT_index].current_position/64)
    {
        case 0:
            read_block(descriptor[OFT[OFT_index].index].block1, OFT[OFT_index].r_w_buffer);
            break;
            
        case 1:
            read_block(descriptor[OFT[OFT_index].index].block2, OFT[OFT_index].r_w_buffer);
            break;
            
        case 2:
            read_block(descriptor[OFT[OFT_index].index].block3, OFT[OFT_index].r_w_buffer);
            break;
    }
}

int find_free_block()
{
    for(int i = 0; i < 2; i++)
        for(int j = 0; j < 32; j++) //find a free block by searching bitmap
        {
            int test = bitmap[i] & MASK[j];
            if(test == 0)
            {
                bitmap[i] = bitmap[i] | MASK[j]; //set the block to 1
                return j;
            }
        }
    return 0;
}

void write_to_directory(char * file_entry)
{
    int OFT_index = 0;
    
    load_buffer(OFT_index);

    switch(OFT[OFT_index].current_position / 64)
    {
        case 0: //writing in block1 buffer
            for(int i = 0; i < 4; i++)
            {
                OFT[OFT_index].r_w_buffer[OFT[OFT_index].current_position] = file_entry[i];
                OFT[OFT_index].current_position++;
            }
            break;
            
        case 1: //block 2
            for(int i = 0; i < 4; i++)
            {
                OFT[OFT_index].r_w_buffer[OFT[OFT_index].current_position-64] = file_entry[i];
                OFT[OFT_index].current_position++;
            }
            break;
            
        case 2:
            for(int i = 0; i < 4; i++)
            {
                OFT[OFT_index].r_w_buffer[OFT[OFT_index].current_position-128] = file_entry[i];
                OFT[OFT_index].current_position++;
            }
            break;
            
        case 3:
            printf("Error\n");
            return;
    }
    
    if(OFT[OFT_index].current_position <= 64)
    {
        int descriptor_index = OFT[OFT_index].index;
        
        //find the correct block and write in it also update the descriptor
        if(descriptor[descriptor_index].block1) //if the block1 already exist
        {
            write_block(descriptor[descriptor_index].block1, OFT[OFT_index].r_w_buffer);
            descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
        }
        else
        {
            if((descriptor[descriptor_index].block1 = find_free_block()))
            {
                write_block(descriptor[descriptor_index].block1, OFT[OFT_index].r_w_buffer);
                descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
            }
            else
                printf("Error\n");
        }
        clear_buffer(OFT_index);
    }
    else if(OFT[OFT_index].current_position / 64 == 1)
    {
        int descriptor_index = OFT[OFT_index].index;
        //find the correct block and write in it also update the descriptor
        if(descriptor[descriptor_index].block2) //if the block1 already exist
        {
            write_block(descriptor[descriptor_index].block2, OFT[OFT_index].r_w_buffer);
            descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
        }
        else
        {
            if((descriptor[descriptor_index].block2 = find_free_block()))
            {
                write_block(descriptor[descriptor_index].block2, OFT[OFT_index].r_w_buffer);
                descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
            }
            else
                printf("Error\n");
        }
        clear_buffer(OFT_index);
    }
    else if(OFT[OFT_index].current_position / 64 == 2)
    {
        int descriptor_index = OFT[OFT_index].index;
        //find the correct block and write in it also update the descriptor
        if(descriptor[descriptor_index].block3) //if the block1 already exist
        {
            write_block(descriptor[descriptor_index].block3, OFT[OFT_index].r_w_buffer);
            descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
        }
        else
        {
            if((descriptor[descriptor_index].block3 = find_free_block()))
            {
                write_block(descriptor[descriptor_index].block3, OFT[OFT_index].r_w_buffer);
                descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
            }
            else
                printf("Fail to allocate the block\n");
        }
        clear_buffer(OFT_index);
    }
}

int create_new_file(char* new_file_name)
{
    for(int i = 0; i < DESCRIPTOR_SIZE-1; i++)  //search existing file
        if(strncmp(new_file_name, &directory[i][1], 3) == 0)
            return 1;
    
    int new_file_descriptor = 0;
    for(int i = 0; i < DESCRIPTOR_SIZE; i++)
        if(descriptor[i].file_length == -1) //the ith descriptor are free
        {
            new_file_descriptor = i;
            descriptor[i].file_length = 0;
            
            //put new file in directory entry
            for(int j = 0; j < DESCRIPTOR_SIZE; j++)
                if(directory[j][0] == 0) // jth entry of directory is free
                {
                    directory[j][0] = new_file_descriptor;
                    char * c = &directory[j][1];
                    strncpy(c, new_file_name, 3);
                    write_to_directory(directory[j]);
                    return 0;
                }
        }
    return 1;
}

int remove_file(int directory_index)
{
    int descriptor_index = directory[directory_index][0];
    for(int i = 0; i < FILE_NAME_LENGTH; i++)
        directory[directory_index][i] = 0;
    
    descriptor[descriptor_index].file_length = -1;
    free_block(descriptor[descriptor_index].block1);
    free_block(descriptor[descriptor_index].block2);
    free_block(descriptor[descriptor_index].block3);
    
    return 0;
}

int open_file(int descriptor_index)
{
    for(int i = 1; i < DESCRIPTOR_SIZE; i++) //OFT[0] reserved for directory
        if(OFT[i].index == -1)
        {
            OFT[i].index = descriptor_index;
            if(descriptor[descriptor_index].block1 != 0)
                read_block(descriptor[descriptor_index].block1, OFT[i].r_w_buffer);
            return i;
        }
    
    printf("Reached here %d\n", OFT[2].index);
    return 0; //here is something different, if the file open false, return 0;
}

int close_file(int cl_OFT_index)
{
    if(OFT[cl_OFT_index].index == -1)
        return 1;
    OFT[cl_OFT_index].current_position = 0;
    clear_buffer(cl_OFT_index);
    OFT[cl_OFT_index].index = -1;
    return 0;
}



int write_to_buffer(int OFT_index, char input, int count)
{
    if(OFT_index < 4 && OFT[OFT_index].index != -1) //if index is in the oft and also opened
    {
        load_buffer(OFT_index);
        while(count--)
        {
            switch(OFT[OFT_index].current_position / 64)
            {
                case 0: //writing in block1 buffer
                    OFT[OFT_index].r_w_buffer[OFT[OFT_index].current_position] = input;
                    OFT[OFT_index].current_position++;
                    break;
                    
                case 1: //block 2
                    OFT[OFT_index].r_w_buffer[OFT[OFT_index].current_position-64] = input;
                    OFT[OFT_index].current_position++;
                    break;
                    
                case 2:
                    OFT[OFT_index].r_w_buffer[OFT[OFT_index].current_position-128] = input;
                    OFT[OFT_index].current_position++;
                    break;
                    
                case 3:
                    printf("Buffer over loaded, file reaches maximum size, only stored first 192 bytes\n");
                    return 0;
            }
            
            if(OFT[OFT_index].current_position == 64 || (count == 0 && OFT[OFT_index].current_position / 64 == 0))
            {
                int descriptor_index = OFT[OFT_index].index;
                
                //find the correct block and write in it also update the descriptor
                if(descriptor[descriptor_index].block1) //if the block1 already exist
                {
                    write_block(descriptor[descriptor_index].block1, OFT[OFT_index].r_w_buffer);
                    descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
                }
                else
                {
                    if((descriptor[descriptor_index].block1 = find_free_block()))
                    {
                        write_block(descriptor[descriptor_index].block1, OFT[OFT_index].r_w_buffer);
                        descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
                    }
                    else
                        printf("Fail to allocate the block\n");
                }
                clear_buffer(OFT_index);
            }
            else if(OFT[OFT_index].current_position == 128 || (count == 0 && OFT[OFT_index].current_position / 64 == 1))
            {
                int descriptor_index = OFT[OFT_index].index;
                //find the correct block and write in it also update the descriptor
                if(descriptor[descriptor_index].block2) //if the block1 already exist
                {
                    write_block(descriptor[descriptor_index].block2, OFT[OFT_index].r_w_buffer);
                    descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
                }
                else
                {
                    if((descriptor[descriptor_index].block2 = find_free_block()))
                    {
                        write_block(descriptor[descriptor_index].block2, OFT[OFT_index].r_w_buffer);
                        descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
                    }
                    else
                        printf("Fail to allocate the block\n");
                }
                clear_buffer(OFT_index);
            }
            else if(OFT[OFT_index].current_position == 192 || (count == 0 && OFT[OFT_index].current_position / 64 == 2))
            {
                int descriptor_index = OFT[OFT_index].index;
                //find the correct block and write in it also update the descriptor
                if(descriptor[descriptor_index].block3) //if the block1 already exist
                {
                    write_block(descriptor[descriptor_index].block3, OFT[OFT_index].r_w_buffer);
                    descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
                }
                else
                {
                    if((descriptor[descriptor_index].block3 = find_free_block()))
                    {
                        write_block(descriptor[descriptor_index].block3, OFT[OFT_index].r_w_buffer);
                        descriptor[descriptor_index].file_length = OFT[OFT_index].current_position;
                    }
                    else
                        printf("Fail to allocate the block\n");
                }
                clear_buffer(OFT_index);
            }
        }
        
        
        descriptor[OFT[OFT_index].index].file_length = OFT[OFT_index].current_position;
        return 0;
    }
    else
        return 1;
}

int seek(int sk_OFT_index, int position)
{
    if(sk_OFT_index > 3 || OFT[sk_OFT_index].index == -1 || descriptor[OFT[sk_OFT_index].index].file_length < position)
        return 1;
    
    OFT[sk_OFT_index].current_position = position;
    return 0;
}

int read_file(int rd_OFT_index, int count)
{
    if(OFT[rd_OFT_index].index == -1 || rd_OFT_index > 3)
        return 1;
    
    int current_position = OFT[rd_OFT_index].current_position;
    while(count > 0)
    {
        switch (OFT[rd_OFT_index].current_position / 64)
        {
            case 0:
                read_block(descriptor[OFT[rd_OFT_index].index].block1, OFT[rd_OFT_index].r_w_buffer);
                for(;count > 0; count--)
                {
                    if(current_position > 63)
                        break;
                    printf("%c", OFT[rd_OFT_index].r_w_buffer[current_position]);
                    current_position++;
                }
                OFT[rd_OFT_index].current_position = current_position;
                break;
                
            case 1:
                read_block(descriptor[OFT[rd_OFT_index].index].block2, OFT[rd_OFT_index].r_w_buffer);
                for(;count > 0; count--)
                {
                    if(current_position > 127)
                        break;
                    printf("%c", OFT[rd_OFT_index].r_w_buffer[current_position-64]);
                    current_position++;
                }
                OFT[rd_OFT_index].current_position = current_position;
                break;
                
            case 2:
                read_block(descriptor[OFT[rd_OFT_index].index].block3, OFT[rd_OFT_index].r_w_buffer);
                for(;count > 0; count--)
                {
                    if(current_position > 191)
                        break;
                    printf("%c", OFT[rd_OFT_index].r_w_buffer[current_position-128]);
                    current_position++;
                }
                OFT[rd_OFT_index].current_position = current_position;
                break;
                
            default:
                printf("Error read\n");
                return 1;
        }
    }
    printf("\n");
    return 0;
}

int save_to_file(char * file_name)
{
    
    FILE * pFILE;
    char path[100] = "/Users/Xiang_Mac/";
    strcat(path, file_name);
    
    if((pFILE = fopen(path, "w")) != NULL)
    {
        //save bitmap
        ldisk[0][0] = bitmap[0];
        ldisk[0][1] = bitmap[1];
        
        //save descriptor
        
        int descriptor_index = 0;
        for(int i = 1; i < 7; i++)
            for(int j = 0; j < 16;)
            {
                ldisk[i][j++] = descriptor[descriptor_index].file_length;
                ldisk[i][j++] = descriptor[descriptor_index].block1;
                ldisk[i][j++] = descriptor[descriptor_index].block2;
                ldisk[i][j++] = descriptor[descriptor_index++].block3;
            }
        
        //clean memory
        initial_OFT();
        initial_bitmap();
        initial_descripor();
        for(int i = 0; i < 63; i++)
            for(int j = 0; j < 5; j++)
                directory[i][j] = 0;
        
        //write into the file
        for(int i = 0; i < 64; i++)
            for(int j = 0; j < 16; j++)
                fprintf(pFILE, "%u\n", ldisk[i][j]);
        
        fclose(pFILE);
    }
    return 0;
}

int restore_disk(char * file_name)
{
    FILE * pFile;
    char path[100] = "/Users/Xiang_Mac/";
    strcat(path, file_name);
    
    if((pFile = fopen(path, "r")) != NULL)
    {
        for(int i = 0; i < 64; i++)
            for(int j = 0; j < 16; j++)
                fscanf(pFile, "%u", &ldisk[i][j]);
        
        //restore bitmap
        bitmap[0] = ldisk[0][0];
        bitmap[1] = ldisk[0][1];
        
        //restore descriptor
        int descriptor_index = 0;
        for(int i = 1; i < 7; i++)
            for(int j = 0; j < 16;)
            {
                descriptor[descriptor_index].file_length = ldisk[i][j++];
                descriptor[descriptor_index].block1 = ldisk[i][j++];
                descriptor[descriptor_index].block2 = ldisk[i][j++];
                descriptor[descriptor_index++].block3 = ldisk[i][j++];
            }
        
        //restore directory
        if(descriptor[0].block1 > 0)
        {
            load_buffer(0);
            
            int buffer_index = 0;
            for(int i = 0; i < 8; i++)
                for(int j = 0; j < 4; j++)
                    directory[i][j] = OFT[0].r_w_buffer[buffer_index++];
        }
        return 0;
            
    }
    else
    {
        printf("Error\n");
        initial_bitmap();
        initial_descripor();
        initial_directory();
        initial_OFT();
        printf("disk initialized\n");
        return 1;
    }

}

int main(int argc, char* argv[])
{
    open_command_file(); //if fail, return 1
    initial_MASK();
 
    char input[100] = "";
    while(fgets(input, 100, command_file))
    {
        if(!strncmp(input, "in", 2))
        {
            int input_length = (int)strlen(input);
            if(input_length > 4)
            {
                char* saved_ldisk_file_path = &input[3]; //initialize by saved ldisk file
                saved_ldisk_file_path[strlen(saved_ldisk_file_path)-2] = '\0';
                if(restore_disk(saved_ldisk_file_path) == 0)
                    printf("disk restored\n"); //saved ldisk file path will have a \n\0 in the end.
            }
            else
            {
                initial_bitmap();
                initial_descripor();
                initial_directory();
                initial_OFT();
                printf("disk initialized\n");
            }
        }
        else if(!strncmp(input, "cr", 2))
        {
            char new_file_name[FILE_NAME_LENGTH]; //since the file name is limited to 3 char
            for(int i = 3; i < 6; i++)
                new_file_name[i-3] = input[i];
            new_file_name[FILE_NAME_LENGTH-1] = '\0';
            
            if(create_new_file(new_file_name) == 0)
                printf("%s created\n", new_file_name);
            else
                printf("Error\n");
        }
        else if(!strncmp(input, "de", 2))
        {
            char de_file_name[FILE_NAME_LENGTH];
            for(int i = 3; i < 6; i++)
                de_file_name[i-3] = input[i];
            de_file_name[FILE_NAME_LENGTH-1] = '\0';

            int found = 0;
            for(int i = 0; i < DESCRIPTOR_SIZE-1; i++)
                if(strncmp(&directory[i][1], de_file_name, 3) == 0)
                {
                    found = 1;
                    if(remove_file(i) == 0)
                        printf("%s destroyed\n", de_file_name);
                }
            if(!found)
                printf("Error\n");
        }
        else if(!strncmp(input, "cl", 2))
        {
            char *tem_string = strtok(&input[3], " "); //get OFT index
            int cl_OFT_index = atoi(tem_string);
            if(close_file(cl_OFT_index) == 0)
                printf("%d closed\n", cl_OFT_index);
            else
                printf("Error\n");
        }
        else if(!strncmp(input, "rd", 2))
        {
            char *tem_string = strtok(&input[3], " "); //get OFT index
            int rd_OFT_index = atoi(tem_string);
            
            tem_string = strtok(NULL, " "); //get count
            int count = atoi(tem_string);
            
            if(read_file(rd_OFT_index, count))
                printf("Error\n");
            
        }
        else if(!strncmp(input, "wr", 2))
        {
            char *tem_string = strtok(&input[3], " "); //get OFT index
            int wr_OFT_index = atoi(tem_string);
            
            tem_string = strtok(NULL, " "); //get input_letter
            char input_letter = tem_string[0];
            
            tem_string = strtok(NULL, " "); //get count
            int count = atoi(tem_string);
            
            if(write_to_buffer(wr_OFT_index, input_letter, count) == 0)
                printf("%d bytes written\n", count);
            else
                printf("Error\n");
            
        }
        else if(!strncmp(input, "sk", 2))
        {
            char *tem_string = strtok(&input[3], " ");
            int sk_OFT_index = atoi(tem_string);
            
            tem_string = strtok(NULL, " ");
            int position = atoi(tem_string);
            
            if(seek(sk_OFT_index, position) == 0)
                printf("Position is %d\n", position);
            else
                printf("Error\n");
        }
        else if(!strncmp(input, "dr", 2))
        {
            for(int i = 0; i < DESCRIPTOR_SIZE-1; i++)
                if(directory[i][0])
                {
                    for(int j = 1; j < 4; j++)
                        printf("%c", directory[i][j]);
                    printf(" ");
                }
            printf("\n");
        }
        else if(!strncmp(input, "sv", 2))
        {
            char *tem_string = strtok(&input[3], " ");
            tem_string[strlen(tem_string)-2] = '\0';
            
            save_to_file(tem_string);
            printf("disk saved\n");
        }
        else if(!strncmp(input, "op", 2))
        {
            char op_file_name[FILE_NAME_LENGTH];
            for(int i = 3; i < 6; i++)
                op_file_name[i-3] = input[i];
            op_file_name[FILE_NAME_LENGTH-1] = '\0';
            
            int file_found = 0;
            for(int i = 0; i < DESCRIPTOR_SIZE-1; i++)  //search directory for file
                if(strncmp(op_file_name, &directory[i][1], 3) == 0)
                {
                    file_found = 1;
                    int op_file_index = 0;
                    if((op_file_index = open_file(directory[i][0])))
                        printf("%s opened %d\n", op_file_name, op_file_index);
                    else
                        printf("Error\n");
                }
            if(!file_found)
                printf("Error\n");
        }
        else
            printf("\n");
    }
    return 0;
}

