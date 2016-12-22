/********************************************************************************
 *                                                                              *
 * this file is part of:                                                        *
 * libeep, the project for reading and writing avr/cnt eeg and related files    *
 *                                                                              *
 ********************************************************************************
 *                                                                              *
 * LICENSE:Copyright (c) 2003-2016,                                             *
 * Advanced Neuro Technology (ANT) B.V., Enschede, The Netherlands              *
 * Max-Planck Institute for Human Cognitive & Brain Sciences, Leipzig, Germany  *
 *                                                                              *
 ********************************************************************************
 *                                                                              *
 * This library is free software; you can redistribute it and/or modify         *
 * it under the terms of the GNU Lesser General Public License as published by  *
 * the Free Software Foundation; either version 3 of the License, or            *
 * (at your option) any later version.                                          *
 *                                                                              *
 * This library is distributed WITHOUT ANY WARRANTY; even the implied warranty  *
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
 * GNU Lesser General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the GNU Lesser General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>          *
 *                                                                              *
 *******************************************************************************/

#include "mex.h"		/* Matlab specific  */
#include "matrix.h"		/* Matlab specific  */

#include <v4/eep.h>	 /* MPI-ANT specific */

void
mexFunction (int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[]) {
  const        matlab_offset_correction = 1;
  char         filename[256];
  int          sample1;
  int          sample2;
  int          channel_count;
  int          sample_count;
  int          trigger_count;
  int          s;
  int          c;
  int          t;
  const char * trigger_label;
  uint64_t     trigger_offset;
  uint64_t     trigger_duration;
  cntfile_t    libeep_handle;
  float      * libeep_sample_data;
  float      * libeep_sample_data_ptr;
  const int    dimensions_1_1[] = {1, 1};
  const int    field_names_count = 2;
  const char * field_names[] = { "samples", "triggers" };
  const int    trigger_field_names_count = 3;
  const char * trigger_field_names[] = { "offset", "code", "duration" };
  mxArray    * mx_sample_data;
  double     * mx_sample_data_ptr;
  int          mx_trigger_count;
  mxArray    * mx_triggers;

  if(nrhs!=3) {
    mexErrMsgTxt("Invalid number of input arguments");
  }

  // get parameters
  mxGetString(prhs[0], filename, 256);
  sample1 = (int)(mxGetScalar(prhs[1])) - matlab_offset_correction;
  sample2 = (int)(mxGetScalar(prhs[2])) - matlab_offset_correction;
  sample_count = sample2 - sample1;

  if(sample2 <= sample1) {
    mexErrMsgTxt("sample1 must be smaller than sample2");
  }

  // create return structure
  plhs[0] = mxCreateStructArray(2, dimensions_1_1, field_names_count, field_names);

  // open
  libeep_handle = libeep_read_with_external_triggers(filename);
  if(libeep_handle == -1) {
    mexErrMsgTxt("Could not open file");
  }

  // samples
  libeep_sample_data = libeep_get_samples(libeep_handle, sample1, sample2);
  if(libeep_sample_data == NULL) {
    mexErrMsgTxt("Error getting samples");
  }
  channel_count = libeep_get_channel_count(libeep_handle);
  mx_sample_data = mxCreateDoubleMatrix(channel_count, sample_count, mxREAL);
  // copy
  libeep_sample_data_ptr = libeep_sample_data;
  mx_sample_data_ptr = mxGetPr(mx_sample_data);
  for(s=0;s<sample_count;++s) {
    for(c=0;c<channel_count;++c) {
      *mx_sample_data_ptr = (double) *libeep_sample_data_ptr;
      ++mx_sample_data_ptr;
      ++libeep_sample_data_ptr;
    }
  }
  libeep_free_samples(libeep_sample_data);
  mxSetField(plhs[0], 0, field_names[0], mx_sample_data);

  // triggers, pass one, count triggers
  trigger_count = 0;
  for(t=0;t<libeep_get_trigger_count(libeep_handle);++t) {
    trigger_label = libeep_get_trigger(libeep_handle, t, &trigger_offset);
    if(trigger_offset >= sample1 && trigger_offset < sample2) {
      ++trigger_count;
    }
  }
  // triggers, pass two, copy triggers
  mx_triggers = mxCreateStructMatrix(1, trigger_count, trigger_field_names_count, trigger_field_names);
  mx_trigger_count = 0;
  for(t=0;t<libeep_get_trigger_count(libeep_handle);++t) {
    trigger_label = libeep_get_trigger_with_duration(libeep_handle, t, &trigger_offset, &trigger_duration);
    if(trigger_offset >= sample1 && trigger_offset < sample2) {
      // copy
      mxArray * offset;
      mxArray * code;
      mxArray * duration;

      offset = mxCreateDoubleMatrix(1,1,mxREAL);
      *mxGetPr(offset) = (double)trigger_offset + matlab_offset_correction - sample1; // correct for section of data we need

      duration = mxCreateDoubleMatrix(1,1,mxREAL);
      *mxGetPr(duration) = (double)trigger_duration;

      code = mxCreateString(trigger_label);

      mxSetField(mx_triggers, mx_trigger_count, "offset", offset);
      mxSetField(mx_triggers, mx_trigger_count, "code", code);
      mxSetField(mx_triggers, mx_trigger_count, "duration", duration);
      ++mx_trigger_count;
    }
  }
  mxSetField(plhs[0], 0, field_names[1], mx_triggers);

  // close
  libeep_close(libeep_handle);
}
