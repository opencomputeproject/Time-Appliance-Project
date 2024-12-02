#ifndef MY_ARM_MATH_H
#define MY_ARM_MATH_H

#include <stdint.h>
#include <math.h>

typedef float float32_t;

#define __STATIC_FORCEINLINE static inline __attribute__((always_inline))

#define PI 3.14159265358979

typedef enum
{
  ARM_MATH_SUCCESS                 =  0,        /**< No error */
  ARM_MATH_ARGUMENT_ERROR          = -1,        /**< One or more arguments are incorrect */
  ARM_MATH_LENGTH_ERROR            = -2,        /**< Length of data buffer is incorrect */
  ARM_MATH_SIZE_MISMATCH           = -3,        /**< Size of matrices is not compatible with the operation */
  ARM_MATH_NANINF                  = -4,        /**< Not-a-number (NaN) or infinity is generated */
  ARM_MATH_SINGULAR                = -5,        /**< Input matrix is singular and cannot be inverted */
  ARM_MATH_TEST_FAILURE            = -6,        /**< Test Failed */
  ARM_MATH_DECOMPOSITION_FAILURE   = -7         /**< Decomposition Failed */
} arm_status;

arm_status arm_atan2_f32(float y,float x,float *result);



/**
  @brief         Floating-point square root function.
  @param[in]     in    input value
  @param[out]    pOut  square root of input value
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : input value is positive
                   - \ref ARM_MATH_ARGUMENT_ERROR : input value is negative; *pOut is set to 0
 */
__STATIC_FORCEINLINE arm_status arm_sqrt_f32(
  const float32_t in,
  float32_t * pOut)
  {
    if (in >= 0.0f)
    {
#if defined ( __CC_ARM )
  #if defined __TARGET_FPU_VFP
      *pOut = __sqrtf(in);
  #else
      *pOut = sqrtf(in);
  #endif

#elif defined ( __ICCARM__ )
  #if defined __ARMVFP__
      __ASM("VSQRT.F32 %0,%1" : "=t"(*pOut) : "t"(in));
  #else
      *pOut = sqrtf(in);
  #endif

#elif defined ( __ARMCC_VERSION ) && ( __ARMCC_VERSION >= 6010050 )
      *pOut = _sqrtf(in);
#elif defined(__GNUC_PYTHON__)
      *pOut = sqrtf(in);
#elif defined ( __GNUC__ )
  #if defined (__VFP_FP__) && !defined(__SOFTFP__)
    __asm__ volatile (
      "vsqrt.f32 %0, %1"
      : "=t"(*pOut)      // Output
      : "t"(in)          // Input
    );

  #else
      *pOut = sqrtf(in);
  #endif
#else
      *pOut = sqrtf(in);
#endif

      return (ARM_MATH_SUCCESS);
    }
    else
    {
      *pOut = 0.0f;
      return (ARM_MATH_ARGUMENT_ERROR);
    }
  }


#endif