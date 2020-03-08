
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "interruption.h"
#include "clock.h"
#include "stdio.h"
#include "syscall.h"
#include "clock.h"
#include "keyboard.h"
#include "hd_driver.h"
#include "memory.h"

using namespace proc;

void TestA()
{
	int i = 0;
	while (1) {
        u8* p = (u8*)(V_MEM_BASE + (console_table[2].current_start_addr + 20 * 80 + 20) * 2);

		*(char*)(p) = '-';
        *(char*)(p+1) = 0x7;
		clock::milli_delay(4000 );

		*(char*)(p) = '\\';
        *(char*)(p+1) = 0x7;
		clock::milli_delay(4000 );

    	*(char*)(p) = '|';
        *(char*)(p+1) = 0x7;
		clock::milli_delay(4000 );

		*(char*)(p) = '/';
        *(char*)(p+1) = 0x7;
		clock::milli_delay(4000 );
	}
}

class Testa
{
    u32 a;
    u32 v;
};

void TestB()
{
	int i = 0x1000;
    printf("[%d %d].", get_pid(), clock::get_ticks());
    fork();
	while(1){
        printf("[%d %d].", get_pid(), clock::get_ticks());
		clock::milli_delay(10000);
	}
}

int strcmp(const char* a, const char* b)
{
    while (*a != '\0' && *a != ' ' && *b != '\0' && *b != ' ') {
        if (*a != *b)   return 0;
        a++; b++;
    }
    if ((*a == '\0' || *a == ' ') && (*b == '\0' || *b == ' ')) return 1;
    return 0;
}

u8 getchar()
{
    u8 ch;
    while ((ch = getch()) == '\0');
    return ch;
}

u32 getline(char* buf)
{
    u8 ch;
    u32 len = 0;
    char output[2] = {'\0','\0'};
    while ((output[0] = getchar()) != '\n') {
        //disp_str(output);
        buf[len++] = output[0];
    } buf[len] = '\0';
    return len;
}

u32 atoi(const char* buf)
{
    u32 i = 0;
    while (*buf != '\0') i = i * 10 + (*buf++ - '0');
    return i;
}


void load(char* buf)
{
    int i = atoi(buf);

    if (!i) {
        printf("[PGM1]: loaded at 0x80000, hd at 0x00000\n");
        printf("        the fetch image\n");
        printf("[PGM2]: loaded at 0x82000, hd at 0x02000\n");
        printf("        cowsay like\n");
        return;
    }

    i -= 1;

    printf("[PGM]: %d\n", i);

    ipc::Message driver_msg;

    char* usr_pgm_lc = (char*)(0x80000 + i * 0x2000);
    driver_msg.POSITION = i * 0x2000;
    driver_msg.CNT      = 10240;
    driver_msg.PROC_NR  = 3;
    driver_msg.BUF      = usr_pgm_lc;
    driver_msg.type = proc::ipc::DEV_READ;
    driver_msg.source   = 6;

    send_recv(ipc::Type::BOTH, 2, &driver_msg);

    proc_table[proc_num - 1].regs.eip = 0x80000 + i * 0x2000;
    proc_table[proc_num - 1].regs.esp = (u32)(proc::proc_stack + proc::kProcStackSize - 7 * 0x4000);
    proc_table[proc_num - 1].state = READY;
}

void show_proc_table()
{
    for (u32 i = 0; i < proc_num; ++i) {
        printf(proc_table[i].name);
        printf(" pid: %d ", proc_table[i].pid);
        printf(" state: %d ", proc_table[i].state);
        printf("\n");
    }
}

void cc(char* input, char* output)
{
    u32 pos = 0;
    while (input[pos] >= '0' && input[pos] <= '9') pos++;
    u32 a, b, c; u8 ch;
    ch = input[pos];
    input[pos] = '\0';
    a = atoi(input);
    b = atoi(input + pos + 1);
    input[pos] = ch;
    switch (ch) {
    case '+': c = a + b; break;
    case '-': c = a - b; break;
    case '*': c = a * b; break;
    case '/': c = a / b; break;
    default: break;
    };
    sprintf(output, "%d", c);
}

void base64_encode(char* input, char* output)
{
    printf("[base64-encode]encoding (%s)\n", input);

    char ot[2] = {'\0', '\0'};
    char CONVERT_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

    u32 cnt = 0;
    const char* sourcebuf = input;
    int buflen = strlen(input);
    char split3[3];
    unsigned int split4[4], len = buflen, i = 0;
    while (len--)
    {
        split3[i++] = *(sourcebuf++);
        if (i == 3)
        {
            split4[0] = (split3[0] & 0xfc) >> 2;							   //第一个字节取前6位并将其右移2位
            split4[1] = ((split3[0] & 0x03) << 4) + ((split3[1] & 0xf0) >> 4); //第一个字节取后2位并左移4位 + 第二个字节取前4位并右移4位
            split4[2] = ((split3[1] & 0x0f) << 2) + ((split3[2] & 0xc0) >> 6); //第二个字节取后4位并左移2位 + 第三个字节取前2位并右移6位
            split4[3] = (split3[2] & 0x3f);									   //第三个字节取后六位并且无需移位
            for (int j = 0; j < 4; ++j) {
                ot[0] = CONVERT_TABLE[split4[j]];
                cnt += sprintf(output + cnt, ot);
            }
            i = 0;
        }
    }
    if (i)
    {
        if (i == 1)
        {
            split4[0] = (split3[0] & 0xfc) >> 2;
            split4[1] = (split3[0] & 0x03 << 4);
            for (int j = 0; j < 2; ++j) {
                ot[0] = CONVERT_TABLE[split4[j]];
                cnt += sprintf(output + cnt, ot);
            }
            cnt += sprintf(output + cnt, "==");
        }
        if (i == 2)
        {
            split4[0] = (split3[0] & 0xfc) >> 2;
            split4[1] = ((split3[0] & 0x03) << 4) + ((split3[1] & 0xf0) >> 4);
            split4[2] = ((split3[1] & 0x0f) << 2);
            for (int j = 0; j < 3; j++) {
                ot[0] = CONVERT_TABLE[split4[j]];
                cnt += sprintf(output + cnt, ot);
            }
            cnt += sprintf(output + cnt, "==");
        }
    }
}

void base64_decode(char* input, char* output)
{
    printf("[base64-decode]encoding (%s)\n", input);
    u32 olen = 0;
    char CONVERT_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
    char* sourcebuf = input;
    const int buflen = strlen(input);

    unsigned char split3[3];
    unsigned char split4[4];
    int len = buflen, ec = 0, seg = buflen / 4; //计算共可以切成几组
    while (sourcebuf[--len] == '=')             //判断末尾有几个等号，由此可知最后四个字节该如何处理
        ++ec;
    for (int k = 0; k < seg; ++k)
    {
        for (int j = 0; j < 4; ++j) //每四字节为一组进行处理
        {
            int index = 0;
            char c = *(sourcebuf++);
            for (int i = 0; i < 64; ++i) //由码索引数
            {
                if (CONVERT_TABLE[i] == c)
                {
                    index = i;
                    break;
                }
            }
            split4[j] = index;
        }
        if (k + 1 == seg)
        {
            if (ec == 2) //当末尾有两个等号时的处理
            {
                split3[0] = (split4[0] << 2) + ((split4[1] & 0x30) >> 4);
                output[olen++] = split3[0];
            }
            else if (ec == 1) //当末尾有一个等号时的处理
            {
                split3[0] = (split4[0] << 2) + ((split4[1] & 0x30) >> 4);
                split3[1] = ((split4[1] & 0x0f) << 4) + ((split4[2] & 0x3c) >> 2);
                output[olen++] = split3[0];
                output[olen++] = split3[1];
            }
            else
            {
                split3[0] = ((split4[0] & 0x3f) << 2) + ((split4[1] & 0x30) >> 4);
                split3[1] = ((split4[1] & 0x0f) << 4) + ((split4[2] & 0x3c) >> 2);
                split3[2] = ((split4[2] & 0x03) << 6) + split4[3];
                for (int j = 0; j < 3; ++j)
                    output[olen++] = split3[j];
            }
        }
        else
        {
            split3[0] = ((split4[0] & 0x3f) << 2) + ((split4[1] & 0x30) >> 4);
            split3[1] = ((split4[1] & 0x0f) << 4) + ((split4[2] & 0x3c) >> 2);
            split3[2] = ((split4[2] & 0x03) << 6) + split4[3];
            for (int j = 0; j < 3; ++j)
                output[olen++] = split3[j];
        }
    }
    output[olen] = '\0';
}

void cow_say(char* input)
{
    u32 len = strlen(input);
    if (len < 41) {
        printf(" "); for (int i = 0; i < len; ++i) printf("_\n");
        printf("|"); for (int i = 0; i < len; ++i) if (*input == '\n') input[i] = ' '; printf(input); printf("|\n");
        printf(" "); for (int i = 0; i < len; ++i) printf("_\n");
    }
    "_________________________________________";
    printf(" _________________________________________");
    printf("/ a.img bochsout.txt bochsrc boot include \\");
    printf("\\ kernel kernel.bin lib Makefile          /");
    printf(" -----------------------------------------");
    printf("        \   ^__^                ");
    printf("         \  (oo)\_______        ");
    printf("            (__)\       )\/\\   ");
    printf("                ||----w |       ");
    printf("                ||     ||       ");
}

#define SHELL_OP_E  0   // end
#define SHELL_OP_C  1   // |
#define SHELL_OP_N  2   // >

static u32  cnt;
static char input[256];
static char output[256];
static char tinput[256];
static char toutput[256];
static char arg[100];
static int  is_arg;

void shell_main()
{
    u32 len = 0;
    //disp_clean();
    while (1) {
        printf("[shadow3002](Ticks: %d)\n", clock::get_ticks());

        getline(input);
        u32 i = 0;
        char* next = input; char* end = input + strlen(input);
        is_arg = 0;
        while (next != end) {
            u8 ch; char* next_ = next;
            while (*next_ != '\0' && *next_ != '|' && *next_ != '>') next_++;
            ch = *next_; *next_ = '\0';
            // for (u32 i = 0; i < cnt; ++i) {
            //     printf("%d: %s\n", i, ops[i]);
            // }
            if (strcmp(next, "clear")) {
                //disp_clean();
            }
            else if (strcmp(next, "help")) {
                printf("Here is help\n");
            }
            else if (strcmp(next, "cc")) {
                cc(next + 3, output);
                printf("%s\n", output);
            }
            else if (strcmp(next, "proc")) {
                show_proc_table();
            }
            else if (strcmp(next, "bse")) {
                if (next[3] == '\0') {printf("\n");}
                base64_encode(next + 4, output);
                printf(output);
                printf("\n");
            }
            else if (strcmp(next, "bsd")) {
                if (next[3] == '\0') {printf("\n"); continue;}
                base64_decode(next + 4, output);
                printf(output);
                printf("\n");
            }
            else if (strcmp(next, "load")) {
                load(next + 5);
                clock::milli_delay(1000);
                proc_table[proc_num-1].state = NEW;
            }
            else if (strcmp(next, "fetch")) {
                printf("                   -`                    shadow3002@bochs\n");
                printf("                  .o+`                   ---------------\n");
                printf("                 `ooo/                   OS: S3OS\n");
                printf("                `+oooo:                  Host: Bochs x86 Emulator 2.6.9\n");
                printf("               `+oooooo:                 Kernel: 3.0.0\n");
                printf("               -+oooooo+:                Ticks: %d\n", clock::get_ticks());
                printf("             `/:-:++oooo+:               Processes: %d\n", proc_num);
                printf("            `/++++/+++++++:              Shell: void\n");
                printf("           `/++++++++++++++:             DE: void\n");
                printf("          `/+++ooooooooooooo/`           WM: void\n");
                printf("         ./ooosssso++osssssso+`          WM Theme: void\n");
                printf("        .oossssso-````/ossssss+`         Theme: void\n");
                printf("       -osssssso.      :ssssssso.        Icons: void\n");
                printf("      :osssssss/        osssso+++.       CPU: I don't know\n");
                printf("     /ossssssss/        +ssssooo/-       GPU: void\n");
                printf("   `/ossssso+/:-        -:/+osssso+-     GPU: void\n");
                printf("  `+sso+:-`                 `.-/+oso:    Memory: 32MB\n");
                printf(" `++:.                           `-/+/\n");
                printf(" .`                                 `/\n");

            }
            else {
                printf("[ERROR]"); printf(next);
                printf("\nError, please input help to get more info.\n");
            }

            switch (ch) {
            case '|': next = next_ + 1; break;
            case '>': next = next_ + 1; break;
            case '\0': next = end; break;
            default: break;
            }
        }

    }
}
