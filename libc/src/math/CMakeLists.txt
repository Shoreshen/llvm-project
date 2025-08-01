add_subdirectory(generic)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${LIBC_TARGET_ARCHITECTURE})
  add_subdirectory(${LIBC_TARGET_ARCHITECTURE})
endif()

function(add_math_entrypoint_object name)
  # We prefer machine specific implementation if available. Hence we check
  # that first and return early if we are able to add an alias target for the
  # machine specific implementation.
  get_fq_target_name("${LIBC_TARGET_ARCHITECTURE}.${name}" fq_machine_specific_target_name)
  if(TARGET ${fq_machine_specific_target_name})
    add_entrypoint_object(
      ${name}
      ALIAS
      DEPENDS
        .${LIBC_TARGET_ARCHITECTURE}.${name}
    )
    return()
  endif()

  get_fq_target_name("generic.${name}" fq_generic_target_name)
  if(TARGET ${fq_generic_target_name})
    add_entrypoint_object(
      ${name}
      ALIAS
      DEPENDS
        .generic.${name}
    )
    return()
  endif()

  # Add a dummy entrypoint object for missing implementations. They will be skipped
  # anyway as there will be no entry for them in the target entrypoints list.
  add_entrypoint_object(
    ${name}
    SRCS
      dummy_srcs
    HDRS
      dummy_hdrs
  )
endfunction()

add_math_entrypoint_object(acos)
add_math_entrypoint_object(acosf)
add_math_entrypoint_object(acosf16)

add_math_entrypoint_object(acosh)
add_math_entrypoint_object(acoshf)
add_math_entrypoint_object(acoshf16)

add_math_entrypoint_object(acospif16)
 
add_math_entrypoint_object(asin)
add_math_entrypoint_object(asinf)
add_math_entrypoint_object(asinf16)

add_math_entrypoint_object(asinh)
add_math_entrypoint_object(asinhf)
add_math_entrypoint_object(asinhf16)

add_math_entrypoint_object(atan)
add_math_entrypoint_object(atanf)
add_math_entrypoint_object(atanf16)

add_math_entrypoint_object(atan2)
add_math_entrypoint_object(atan2f)
add_math_entrypoint_object(atan2l)
add_math_entrypoint_object(atan2f128)

add_math_entrypoint_object(atanh)
add_math_entrypoint_object(atanhf)
add_math_entrypoint_object(atanhf16)

add_math_entrypoint_object(canonicalize)
add_math_entrypoint_object(canonicalizef)
add_math_entrypoint_object(canonicalizel)
add_math_entrypoint_object(canonicalizef16)
add_math_entrypoint_object(canonicalizef128)

add_math_entrypoint_object(iscanonical)
add_math_entrypoint_object(iscanonicalf)
add_math_entrypoint_object(iscanonicall)
add_math_entrypoint_object(iscanonicalf16)
add_math_entrypoint_object(iscanonicalf128)

add_math_entrypoint_object(cbrt)
add_math_entrypoint_object(cbrtf)

add_math_entrypoint_object(ceil)
add_math_entrypoint_object(ceilf)
add_math_entrypoint_object(ceill)
add_math_entrypoint_object(ceilf16)
add_math_entrypoint_object(ceilf128)

add_math_entrypoint_object(copysign)
add_math_entrypoint_object(copysignf)
add_math_entrypoint_object(copysignl)
add_math_entrypoint_object(copysignf16)
add_math_entrypoint_object(copysignf128)

add_math_entrypoint_object(cos)
add_math_entrypoint_object(cosf)
add_math_entrypoint_object(cosf16)

add_math_entrypoint_object(cosh)
add_math_entrypoint_object(coshf)
add_math_entrypoint_object(coshf16)

add_math_entrypoint_object(cospif)
add_math_entrypoint_object(cospif16)

add_math_entrypoint_object(daddl)
add_math_entrypoint_object(daddf128)
add_math_entrypoint_object(ddivl)
add_math_entrypoint_object(ddivf128)
add_math_entrypoint_object(dmull)
add_math_entrypoint_object(dmulf128)

add_math_entrypoint_object(dfmal)
add_math_entrypoint_object(dfmaf128)

add_math_entrypoint_object(dsqrtl)
add_math_entrypoint_object(dsqrtf128)

add_math_entrypoint_object(dsubl)
add_math_entrypoint_object(dsubf128)

add_math_entrypoint_object(erf)
add_math_entrypoint_object(erff)

add_math_entrypoint_object(exp)
add_math_entrypoint_object(expf)
add_math_entrypoint_object(expf16)

add_math_entrypoint_object(exp2)
add_math_entrypoint_object(exp2f)
add_math_entrypoint_object(exp2f16)

add_math_entrypoint_object(exp2m1f)
add_math_entrypoint_object(exp2m1f16)

add_math_entrypoint_object(exp10)
add_math_entrypoint_object(exp10f)
add_math_entrypoint_object(exp10f16)

add_math_entrypoint_object(exp10m1f)
add_math_entrypoint_object(exp10m1f16)

add_math_entrypoint_object(expm1)
add_math_entrypoint_object(expm1f)
add_math_entrypoint_object(expm1f16)

add_math_entrypoint_object(f16add)
add_math_entrypoint_object(f16addf)
add_math_entrypoint_object(f16addl)
add_math_entrypoint_object(f16addf128)

add_math_entrypoint_object(f16div)
add_math_entrypoint_object(f16divf)
add_math_entrypoint_object(f16divl)
add_math_entrypoint_object(f16divf128)

add_math_entrypoint_object(f16fma)
add_math_entrypoint_object(f16fmaf)
add_math_entrypoint_object(f16fmal)
add_math_entrypoint_object(f16fmaf128)

add_math_entrypoint_object(f16mul)
add_math_entrypoint_object(f16mulf)
add_math_entrypoint_object(f16mull)
add_math_entrypoint_object(f16mulf128)

add_math_entrypoint_object(f16sqrt)
add_math_entrypoint_object(f16sqrtf)
add_math_entrypoint_object(f16sqrtl)
add_math_entrypoint_object(f16sqrtf128)

add_math_entrypoint_object(fsqrt)
add_math_entrypoint_object(fsqrtl)
add_math_entrypoint_object(fsqrtf128)

add_math_entrypoint_object(f16sub)
add_math_entrypoint_object(f16subf)
add_math_entrypoint_object(f16subl)
add_math_entrypoint_object(f16subf128)

add_math_entrypoint_object(fabs)
add_math_entrypoint_object(fabsf)
add_math_entrypoint_object(fabsl)
add_math_entrypoint_object(fabsf16)
add_math_entrypoint_object(fabsf128)
add_math_entrypoint_object(fabsbf16)

add_math_entrypoint_object(fadd)
add_math_entrypoint_object(faddl)
add_math_entrypoint_object(faddf128)

add_math_entrypoint_object(fdim)
add_math_entrypoint_object(fdimf)
add_math_entrypoint_object(fdiml)
add_math_entrypoint_object(fdimf16)
add_math_entrypoint_object(fdimf128)

add_math_entrypoint_object(fdiv)
add_math_entrypoint_object(fdivl)
add_math_entrypoint_object(fdivf128)

add_math_entrypoint_object(ffma)
add_math_entrypoint_object(ffmal)
add_math_entrypoint_object(ffmaf128)

add_math_entrypoint_object(floor)
add_math_entrypoint_object(floorf)
add_math_entrypoint_object(floorl)
add_math_entrypoint_object(floorf16)
add_math_entrypoint_object(floorf128)

add_math_entrypoint_object(fma)
add_math_entrypoint_object(fmaf)
add_math_entrypoint_object(fmaf16)

add_math_entrypoint_object(fmax)
add_math_entrypoint_object(fmaxf)
add_math_entrypoint_object(fmaxl)
add_math_entrypoint_object(fmaxf128)
add_math_entrypoint_object(fmaxf16)

add_math_entrypoint_object(fmin)
add_math_entrypoint_object(fminf)
add_math_entrypoint_object(fminl)
add_math_entrypoint_object(fminf128)
add_math_entrypoint_object(fminf16)

add_math_entrypoint_object(fmaximum)
add_math_entrypoint_object(fmaximumf)
add_math_entrypoint_object(fmaximuml)
add_math_entrypoint_object(fmaximumf16)
add_math_entrypoint_object(fmaximumf128)

add_math_entrypoint_object(fmaximum_num)
add_math_entrypoint_object(fmaximum_numf)
add_math_entrypoint_object(fmaximum_numl)
add_math_entrypoint_object(fmaximum_numf16)
add_math_entrypoint_object(fmaximum_numf128)

add_math_entrypoint_object(fmaximum_mag)
add_math_entrypoint_object(fmaximum_magf)
add_math_entrypoint_object(fmaximum_magl)
add_math_entrypoint_object(fmaximum_magf16)
add_math_entrypoint_object(fmaximum_magf128)

add_math_entrypoint_object(fmaximum_mag_num)
add_math_entrypoint_object(fmaximum_mag_numf)
add_math_entrypoint_object(fmaximum_mag_numl)
add_math_entrypoint_object(fmaximum_mag_numf16)
add_math_entrypoint_object(fmaximum_mag_numf128)

add_math_entrypoint_object(fminimum)
add_math_entrypoint_object(fminimumf)
add_math_entrypoint_object(fminimuml)
add_math_entrypoint_object(fminimumf16)
add_math_entrypoint_object(fminimumf128)

add_math_entrypoint_object(fminimum_num)
add_math_entrypoint_object(fminimum_numf)
add_math_entrypoint_object(fminimum_numl)
add_math_entrypoint_object(fminimum_numf16)
add_math_entrypoint_object(fminimum_numf128)

add_math_entrypoint_object(fminimum_mag)
add_math_entrypoint_object(fminimum_magf)
add_math_entrypoint_object(fminimum_magl)
add_math_entrypoint_object(fminimum_magf16)
add_math_entrypoint_object(fminimum_magf128)

add_math_entrypoint_object(fminimum_mag_num)
add_math_entrypoint_object(fminimum_mag_numf)
add_math_entrypoint_object(fminimum_mag_numl)
add_math_entrypoint_object(fminimum_mag_numf16)
add_math_entrypoint_object(fminimum_mag_numf128)

add_math_entrypoint_object(fmul)
add_math_entrypoint_object(fmull)
add_math_entrypoint_object(fmulf128)

add_math_entrypoint_object(fmod)
add_math_entrypoint_object(fmodf)
add_math_entrypoint_object(fmodl)
add_math_entrypoint_object(fmodf16)
add_math_entrypoint_object(fmodf128)

add_math_entrypoint_object(frexp)
add_math_entrypoint_object(frexpf)
add_math_entrypoint_object(frexpl)
add_math_entrypoint_object(frexpf16)
add_math_entrypoint_object(frexpf128)

add_math_entrypoint_object(fromfp)
add_math_entrypoint_object(fromfpf)
add_math_entrypoint_object(fromfpl)
add_math_entrypoint_object(fromfpf16)
add_math_entrypoint_object(fromfpf128)

add_math_entrypoint_object(fromfpx)
add_math_entrypoint_object(fromfpxf)
add_math_entrypoint_object(fromfpxl)
add_math_entrypoint_object(fromfpxf16)
add_math_entrypoint_object(fromfpxf128)

add_math_entrypoint_object(fsub)
add_math_entrypoint_object(fsubl)
add_math_entrypoint_object(fsubf128)

add_math_entrypoint_object(getpayload)
add_math_entrypoint_object(getpayloadf)
add_math_entrypoint_object(getpayloadl)
add_math_entrypoint_object(getpayloadf16)
add_math_entrypoint_object(getpayloadf128)

add_math_entrypoint_object(hypot)
add_math_entrypoint_object(hypotf)
add_math_entrypoint_object(hypotf16)

add_math_entrypoint_object(ilogb)
add_math_entrypoint_object(ilogbf)
add_math_entrypoint_object(ilogbl)
add_math_entrypoint_object(ilogbf16)
add_math_entrypoint_object(ilogbf128)

add_math_entrypoint_object(isnan)
add_math_entrypoint_object(isnanf)
add_math_entrypoint_object(isnanl)

add_math_entrypoint_object(issignaling)
add_math_entrypoint_object(issignalingf)
add_math_entrypoint_object(issignalingl)
add_math_entrypoint_object(issignalingf16)
add_math_entrypoint_object(issignalingf128)

add_math_entrypoint_object(llogb)
add_math_entrypoint_object(llogbf)
add_math_entrypoint_object(llogbl)
add_math_entrypoint_object(llogbf16)
add_math_entrypoint_object(llogbf128)

add_math_entrypoint_object(ldexp)
add_math_entrypoint_object(ldexpf)
add_math_entrypoint_object(ldexpl)
add_math_entrypoint_object(ldexpf16)
add_math_entrypoint_object(ldexpf128)

add_math_entrypoint_object(log10)
add_math_entrypoint_object(log10f)
add_math_entrypoint_object(log10f16)

add_math_entrypoint_object(log1p)
add_math_entrypoint_object(log1pf)

add_math_entrypoint_object(log2)
add_math_entrypoint_object(log2f)
add_math_entrypoint_object(log2f16)

add_math_entrypoint_object(log)
add_math_entrypoint_object(logf)
add_math_entrypoint_object(logf16)

add_math_entrypoint_object(logb)
add_math_entrypoint_object(logbf)
add_math_entrypoint_object(logbl)
add_math_entrypoint_object(logbf16)
add_math_entrypoint_object(logbf128)

add_math_entrypoint_object(llrint)
add_math_entrypoint_object(llrintf)
add_math_entrypoint_object(llrintl)
add_math_entrypoint_object(llrintf16)
add_math_entrypoint_object(llrintf128)

add_math_entrypoint_object(llround)
add_math_entrypoint_object(llroundf)
add_math_entrypoint_object(llroundl)
add_math_entrypoint_object(llroundf16)
add_math_entrypoint_object(llroundf128)

add_math_entrypoint_object(lrint)
add_math_entrypoint_object(lrintf)
add_math_entrypoint_object(lrintl)
add_math_entrypoint_object(lrintf16)
add_math_entrypoint_object(lrintf128)

add_math_entrypoint_object(lround)
add_math_entrypoint_object(lroundf)
add_math_entrypoint_object(lroundl)
add_math_entrypoint_object(lroundf16)
add_math_entrypoint_object(lroundf128)

add_math_entrypoint_object(modf)
add_math_entrypoint_object(modff)
add_math_entrypoint_object(modfl)
add_math_entrypoint_object(modff16)
add_math_entrypoint_object(modff128)

add_math_entrypoint_object(nan)
add_math_entrypoint_object(nanf)
add_math_entrypoint_object(nanl)
add_math_entrypoint_object(nanf16)
add_math_entrypoint_object(nanf128)

add_math_entrypoint_object(nearbyint)
add_math_entrypoint_object(nearbyintf)
add_math_entrypoint_object(nearbyintl)
add_math_entrypoint_object(nearbyintf16)
add_math_entrypoint_object(nearbyintf128)

add_math_entrypoint_object(nextafter)
add_math_entrypoint_object(nextafterf)
add_math_entrypoint_object(nextafterl)
add_math_entrypoint_object(nextafterf16)
add_math_entrypoint_object(nextafterf128)

add_math_entrypoint_object(nexttoward)
add_math_entrypoint_object(nexttowardf)
add_math_entrypoint_object(nexttowardl)
add_math_entrypoint_object(nexttowardf16)

add_math_entrypoint_object(nextdown)
add_math_entrypoint_object(nextdownf)
add_math_entrypoint_object(nextdownl)
add_math_entrypoint_object(nextdownf16)
add_math_entrypoint_object(nextdownf128)

add_math_entrypoint_object(nextup)
add_math_entrypoint_object(nextupf)
add_math_entrypoint_object(nextupl)
add_math_entrypoint_object(nextupf16)
add_math_entrypoint_object(nextupf128)

add_math_entrypoint_object(pow)
add_math_entrypoint_object(powf)
add_math_entrypoint_object(powi)
add_math_entrypoint_object(powif)

add_math_entrypoint_object(remainder)
add_math_entrypoint_object(remainderf)
add_math_entrypoint_object(remainderl)
add_math_entrypoint_object(remainderf16)
add_math_entrypoint_object(remainderf128)

add_math_entrypoint_object(remquo)
add_math_entrypoint_object(remquof)
add_math_entrypoint_object(remquof128)
add_math_entrypoint_object(remquol)
add_math_entrypoint_object(remquof16)

add_math_entrypoint_object(rint)
add_math_entrypoint_object(rintf)
add_math_entrypoint_object(rintl)
add_math_entrypoint_object(rintf16)
add_math_entrypoint_object(rintf128)

add_math_entrypoint_object(round)
add_math_entrypoint_object(roundf)
add_math_entrypoint_object(roundl)
add_math_entrypoint_object(roundf16)
add_math_entrypoint_object(roundf128)

add_math_entrypoint_object(roundeven)
add_math_entrypoint_object(roundevenf)
add_math_entrypoint_object(roundevenl)
add_math_entrypoint_object(roundevenf16)
add_math_entrypoint_object(roundevenf128)

add_math_entrypoint_object(scalbln)
add_math_entrypoint_object(scalblnf)
add_math_entrypoint_object(scalblnl)
add_math_entrypoint_object(scalblnf16)
add_math_entrypoint_object(scalblnf128)

add_math_entrypoint_object(scalbn)
add_math_entrypoint_object(scalbnf)
add_math_entrypoint_object(scalbnl)
add_math_entrypoint_object(scalbnf16)
add_math_entrypoint_object(scalbnf128)

add_math_entrypoint_object(setpayload)
add_math_entrypoint_object(setpayloadf)
add_math_entrypoint_object(setpayloadl)
add_math_entrypoint_object(setpayloadf16)
add_math_entrypoint_object(setpayloadf128)

add_math_entrypoint_object(setpayloadsig)
add_math_entrypoint_object(setpayloadsigf)
add_math_entrypoint_object(setpayloadsigl)
add_math_entrypoint_object(setpayloadsigf16)
add_math_entrypoint_object(setpayloadsigf128)

add_math_entrypoint_object(sincos)
add_math_entrypoint_object(sincosf)

add_math_entrypoint_object(sin)
add_math_entrypoint_object(sinf)
add_math_entrypoint_object(sinf16)
add_math_entrypoint_object(sinpif)
add_math_entrypoint_object(sinpif16)

add_math_entrypoint_object(sinh)
add_math_entrypoint_object(sinhf)
add_math_entrypoint_object(sinhf16)

add_math_entrypoint_object(sqrt)
add_math_entrypoint_object(sqrtf)
add_math_entrypoint_object(sqrtl)
add_math_entrypoint_object(sqrtf16)
add_math_entrypoint_object(sqrtf128)

add_math_entrypoint_object(tan)
add_math_entrypoint_object(tanf)
add_math_entrypoint_object(tanf16)

add_math_entrypoint_object(tanh)
add_math_entrypoint_object(tanhf)
add_math_entrypoint_object(tanhf16)

add_math_entrypoint_object(tanpif)
add_math_entrypoint_object(tanpif16)

add_math_entrypoint_object(tgamma)
add_math_entrypoint_object(tgammaf)
add_math_entrypoint_object(lgamma)
add_math_entrypoint_object(lgamma_r)

add_math_entrypoint_object(totalorder)
add_math_entrypoint_object(totalorderf)
add_math_entrypoint_object(totalorderl)
add_math_entrypoint_object(totalorderf16)
add_math_entrypoint_object(totalorderf128)

add_math_entrypoint_object(totalordermag)
add_math_entrypoint_object(totalordermagf)
add_math_entrypoint_object(totalordermagl)
add_math_entrypoint_object(totalordermagf16)
add_math_entrypoint_object(totalordermagf128)

add_math_entrypoint_object(trunc)
add_math_entrypoint_object(truncf)
add_math_entrypoint_object(truncl)
add_math_entrypoint_object(truncf16)
add_math_entrypoint_object(truncf128)

add_math_entrypoint_object(ufromfp)
add_math_entrypoint_object(ufromfpf)
add_math_entrypoint_object(ufromfpl)
add_math_entrypoint_object(ufromfpf16)
add_math_entrypoint_object(ufromfpf128)

add_math_entrypoint_object(ufromfpx)
add_math_entrypoint_object(ufromfpxf)
add_math_entrypoint_object(ufromfpxl)
add_math_entrypoint_object(ufromfpxf16)
add_math_entrypoint_object(ufromfpxf128)
