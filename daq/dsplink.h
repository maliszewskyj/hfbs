#ifndef _dsplink_h
#define _dsplink_h

int init_sis3600(void);
int init_sis3801(int mode);
int reset_sis3600(void);
int reset_sis3801(void);
int init_vadm(void);
int dsp_reset(void);
int dsp_load(int);
int dsp_stat(void);
int load_lookup(char* load_path,int load_len,unsigned int load_base);
int read_rings(void);
int sis3600_read(int * val);

int init_vsc16(void);
int start_vsc16(unsigned int);
int stop_vsc16(void);
int read_vsc16(void);
int enc_omega(void);

void initrun(void);
void loadlook(void);
void buildtlook(void);
void enc_lookup(void);

void dmp_regs(void);
void dmp_look(void);
#endif
