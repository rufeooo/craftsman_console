#include <stdio.h>

int artifactPower = 0;

int enchant() {
  ++artifactPower;
  return artifactPower;
}

int disenchant() {
  --artifactPower;
  return artifactPower;
}
