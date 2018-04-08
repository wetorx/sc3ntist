#include "SC3Argument.h"

SC3Argument SC3ArgumentFromByte(const std::string& name, uint8_t byteValue) {
  SC3Argument arg = {SC3ArgumentType::Byte};
  arg.name = name;
  arg.byteValue = byteValue;
  return arg;
}