#ifndef LOGGER_H_
#define LOGGER_H_

#include <inttypes.h>

void logSetFile(char *filename);
void logMessage(char *message);
void logAddDCS(const uint16_t *data, const unsigned int dcs, const unsigned int width, const unsigned int height);
void logMoveCursor();
int logEnableLogData(const unsigned int state);
int logIsEnabled();
void logCSV();
void logTrigger();

#endif
