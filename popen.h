#ifndef __POPEN_H__
#define __POPEN_H__

FILE *popen_shell(const char *shell, const char *program, const char *type);

int pclose_shell(FILE *iop);

#endif /* __POPEN_H__ */
