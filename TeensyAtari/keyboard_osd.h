
#ifndef keyboard_osd_h_
#define keyboard_osd_h_
    
// extern bool virtualkeyboardIsActive(void);

extern bool menuActive(void);
extern char * menuSelection(void);
extern void toggleMenu(bool on);
extern int  handleMenu(uint16_t bClick);


#endif
