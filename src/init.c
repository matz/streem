#include "strm.h"

void strm_number_init(strm_state* state);
void strm_array_init(strm_state* state);
void strm_string_init(strm_state* state);
void strm_latch_init(strm_state* state);
void strm_iter_init(strm_state* state);
void strm_socket_init(strm_state* state);
void strm_csv_init(strm_state* state);
void strm_kvs_init(strm_state* state);
void strm_time_init(strm_state* state);
void strm_math_init(strm_state* state);

void
strm_init(strm_state* state)
{
  strm_number_init(state);
  strm_array_init(state);
  strm_string_init(state);
  strm_latch_init(state);
  strm_iter_init(state);
  strm_socket_init(state);
  strm_csv_init(state);
  strm_kvs_init(state);
  strm_time_init(state);
  strm_math_init(state);
}
