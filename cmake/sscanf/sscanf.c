#include <stdio.h>
int main( int argc, char** argv, char** env ) {
    int v;
    int r = sscanf("0x00A","%x",&v);
    if(v!=10 || r!=1){ return 1; }
    r = sscanf("0xc9","%x",&v);
    if(v!=201 || r!=1){ return 2; }
    r = sscanf("0xF0","%x",&v);
    if(v!=240 || r!=1){ return 3; }
    r = sscanf("0xFF","%x",&v);
    if(v!=255 || r!=1){ return 4; }
    return 0;
}
