#ifndef _FUNCTIONS_H_
  #define _FUNCTIONS_H_

  #define repeat(n) for(int i = n; i--;)
  #define len(object) sizeof(object) / sizeof(*object)


  uint16_t calculateTicsTime(uint16_t realTime)
    {
      return realTime / portTICK_PERIOD_MS;
    };

#endif