//
//  main.cpp
//  markdown
//
//  Created by Kevin Hira on 8/11/15.
//
//

extern "C"
{
#include "LibStack.h"
}

#include <iostream>
#include <cstdio>
#include <cstring>

#define PARAGRAPH 0x01

#define QBLOCK 0x02
#define CBLOCK 0x03

#define H1 0x0A
#define H2 0x0B
#define H3 0x0C
#define H4 0x0D
#define H5 0x0E
#define H6 0x0F


void Indent(FILE *f, int n);
char *StripNL(char *s);
int ResolveBlock(char *s);
void StartBlock(int block, char *className, char *styles);
void TerminateBlock(void);
void WriteLine(char *s);
void TerminateLine(void);
int IsWhitespace(char c);
int IsListElement(char *s);

char const *tags[] = {"", "p", "blockquote", "code", "pre", "", "", "", "", "", "h1", "h2", "h3", "h4", "h5", "h6"};
int const baseIndent = 2;

int currentBlock = 0;
int currentLine = 0;
int indentN = 0;
int allowChanges = 0;
FILE *outFile, *markdownFile;

int main(int argc, const char * argv[])
{
    
//    char header1[] = "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<title>{TEST}</title>\n";
//    char header2[] = "\t</head>\n\t<body>\n";
//    char footer[] = "\t</body>\n</html>";
    char line[1024];
    int trimStart = 0;
    char *documentTitle = NULL;
    
    if (argc<3) {
        std::cout << "Too few arguments. Usage: " << argv[0] << " fOut fIn [style1 style2 ...]\n";
        return 1;
    }
    if ((markdownFile=fopen(argv[2], "r"))==NULL) {
        std::cout << "Error opening " << argv[2] << " for reading\n";
        return 1;
    }
    if ((outFile=fopen(argv[1], "w"))==NULL) {
        std::cout << "Error opening " << argv[1] << " for writing\n";
        fclose(markdownFile);
        return 1;
    }
    
    fgets(line, 1024, markdownFile);
    if (!strncmp(line, "@@ ", 3)) {
        documentTitle = line+3;
    }
    
    fprintf(outFile, "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<title>%s</title>\n", documentTitle!=NULL?StripNL(documentTitle):"**Untitled**");
    for (int i=argc-1; i>2; --i)
        fprintf(outFile, "\t\t<link rel=\"stylesheet\" href=\"%s\" type=\"text/css\" />\n", argv[i]);
    fprintf(outFile, "\t</head>\n\t<body>\n");
    
    if (documentTitle)
        fgets(line, 1024, markdownFile);
    
     do {
        ++currentLine;
        if (currentBlock==CBLOCK && strncmp(line, "```", 3)) {
            allowChanges = 0;
            ResolveBlock(line);
            WriteLine(line);
            fprintf(outFile, "\n");
            continue;
        }
        allowChanges = 1;
        trimStart = ResolveBlock(line);
        if (trimStart>=0) {
            Indent(outFile, indentN);
            WriteLine(line+trimStart);
            TerminateLine();
        }
     } while (fgets(line, 1024, markdownFile));
    allowChanges = 1;
    TerminateBlock();
    fprintf(outFile, "\t</body>\n</html>");
    
    fclose(outFile);
    fclose(markdownFile);
    return 0;
}

void Indent(FILE *f, int n)
{
    for (int i=0; i<n+baseIndent; i++) {
        fprintf(f, "\t");
    }
}

char *StripNL(char *s)
{
    for (int i=(int)strlen(s)-1; i >=0; --i) {
        if (s[i]=='\n') {
            s[i] = '\0';
            break;
        }
    }
    return s;
}

int ResolveBlock(char *s)
{
    int modifier = 0;
    int changeBlock = 0;
    int newBlock = 0;
    char className[64] = {0};
    char styles[1024] = {0};
    char *p;
    
    switch (s[0]) {
        case '\n':
            if (!strcmp(s, "\n")) {
                changeBlock = 1;
            }
            modifier = -1;
            break;
        case '>':
            modifier = 1;
            if (currentBlock!=QBLOCK) {
                changeBlock = 2;
                newBlock = QBLOCK;
            }
            break;
        case '`':
            if (!strncmp(s, "```", 3)) {
                if (currentBlock==CBLOCK) {
                    changeBlock = 1;
                }
                else {
                    changeBlock = 2;
                    newBlock = CBLOCK;
                }
            }
            modifier = -1;
            break;
        case '#':
            modifier = 1;
            for (int i=1; i<6; ++i) {
                if (s[i] != '#')
                    break;
                ++modifier;
            }
            changeBlock = 2;
            newBlock = H1+(modifier-1);
            break;
        default:
            if (currentBlock>1 && currentBlock!=CBLOCK) {
                changeBlock = 1;
            }
            if (!currentBlock) {
                ++changeBlock;
                newBlock = PARAGRAPH;
            }
            break;
    }
    
    if (modifier>=0) {
        if (s[modifier]=='[' && (p=strchr(s+modifier, ']'))!=NULL) {
            strncpy(className, s+modifier+1, p-(s+modifier+1));
            modifier = (int)(p-s)+1;
        }
        
        if (s[modifier]=='{' && (p=strchr(s+modifier, '}'))!=NULL) {
            strncpy(styles, s+modifier+1, p-(s+modifier+1));
            modifier = (int)(p-s)+1;
        }
        
        if (currentBlock!=CBLOCK && modifier>=0 && s[modifier]==' ')
            ++modifier;
    }
    
    
    
    
    if (changeBlock) {
        TerminateBlock();
    }
    if (newBlock) {
        StartBlock(newBlock, className, styles);
    }
    allowChanges = 0;
    return modifier;
}

void StartBlock(int block, char *className, char *styles)
{
    int isClass = strcmp(className, "");
    int isStyles = strcmp(styles, "");
    if (allowChanges) {
        Indent(outFile, indentN++);
        if (block == CBLOCK) {
            fprintf(outFile, "<%s>\n", tags[block+1]);
            Indent(outFile, indentN++);
        }
        fprintf(outFile, "<%s%s%s%s%s%s%s>\n", tags[currentBlock=block], isClass?" class=\"":"", isClass?className:"", isClass?"\"":"", isStyles?" style=\"":"", isStyles?styles:"", isStyles?"\"":"");
    }
}

void TerminateBlock(void)
{
    if (allowChanges && currentBlock) {
        Indent(outFile, --indentN);
        fprintf(outFile, "</%s>\n", tags[currentBlock]);
        if (currentBlock == CBLOCK) {
            Indent(outFile, --indentN);
            fprintf(outFile, "</%s>\n", tags[currentBlock+1]);
        }
        currentBlock = 0;
    }
}

void WriteLine(char *s)
{
    int isBold = 0, isItalic = 0, isCode = 0, isBL = 0, isStrike = 0;
    
    StripNL(s);
    
    if (currentBlock==CBLOCK) {
        for (int i=0; i<strlen(s); ++i) {
            switch (s[i]) {
                case '<':
                    fprintf(outFile, "&lt;");
                    break;
                case '>':
                    fprintf(outFile, "&gt;");
                    break;
                case '&':
                    fprintf(outFile, "&amp;");
                    break;
                default:
                    fputc(s[i], outFile);
                    break;
            }
        }
        return;
    }
    
    for (int i=0; i<strlen(s); ++i) {
        if (!isCode || s[i]=='`') {
            switch (s[i]) {
                case '\\':
                    fputc(s[++i], outFile);
                    break;
                case '`':
                    fprintf(outFile, "<%scode%s>", isCode?"/":"", isCode?"":" class=\"code-inline\"");
                    isCode = !isCode;
                    break;
                case '_':
                    if (!strncmp(s+i, "_**", 3) && !isBL) {
                        fprintf(outFile, "<span class=\"span-bold span-italic\" style=\"font-weight: bold; font-style: italic\">");
                        i += 2;
                        isBL = 1;
                    }
                    break;
                case '*':
                    if (!strncmp(s+i, "**_", 3) && isBL) {
                        fprintf(outFile, "</span>");
                        i += 2;
                        isBL = 0;
                    }
                    else if (i+1<strlen(s) && s[i+1]=='*') {
                        if (isBold)
                            fprintf(outFile, "</span>");
                        else
                            fprintf(outFile, "<span class=\"span-bold\" style=\"font-weight: bold\">");
                        isBold = !isBold;
                        ++i;
                    }
                    else {
                        if (isItalic)
                            fprintf(outFile, "</span>");
                        else
                            fprintf(outFile, "<span class=\"span-italic\" style=\"font-style: italic\">");
                        isItalic = !isItalic;
                    }
                    break;
                case '~':
                    if (i+1<strlen(s) && s[i+1]=='~') {
                        if (isStrike)
                            fprintf(outFile, "</span>");
                        else
                            fprintf(outFile, "<span class=\"span-strikethough\" style=\"text-decoration: line-through\">");
                        isStrike = !isStrike;
                        ++i;
                    }
                    break;
                default:
                    fputc(s[i], outFile);
                    break;
            }
        }
        else
            fputc(s[i], outFile);
    }
    
    for (int i=0; i<isBold+isItalic+isBL+isStrike; ++i) {
        fprintf(outFile, "</span>");
    }
    
    if (isCode) {
        std::cout << "WARNING (Line " << currentLine << ") -> No closing `\n";
    }
}

void TerminateLine(void)
{
    char nextLine[16];
    int nextStart;
    fpos_t cursor;
    
    fgetpos(markdownFile, &cursor);
    
    fgets(nextLine, 16, markdownFile);
    
    if (feof(markdownFile)) {
        fprintf(outFile, "\n");
    }
    else {
        nextStart = ResolveBlock(nextLine);
        if (currentBlock==PARAGRAPH) {
            if (nextStart==0)
                fprintf(outFile, "<br />\n");
            else if (nextStart!=0)
                fprintf(outFile, "\n");
        }
        else {
            fprintf(outFile, "\n");
        }
    }
    fsetpos(markdownFile, &cursor);
}

int IsWhitespace(char c)
{
    char whiteChars[] = {0x20, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0};
    for (int i=0; i<strlen(whiteChars); ++i) {
        if (c==whiteChars[i])
            return 1;
    }
    return 0;
}

int IsListElement(char *s)
{
    int offset = 0;
    while (IsWhitespace(s[offset]))
        ++offset;
    return -1;
}
