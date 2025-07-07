#pragma once

#include <SD.h>

extern File myFile;
extern bool sdInit;

void initSDCard();

void writeToSettings(String str);