;
; Input signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; SV_Position              0   xyzw        0      POS   float   xy  
;
;
; Output signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; SV_Target                0   xyzw        0   TARGET   float   xyzw
;
; shader hash: 4bd40dcf1d425c9b57f5f5ca98445376
;
; Pipeline Runtime Information: 
;
;PSVRuntimeInfo:
; Pixel Shader
; DepthOutput=0
; SampleFrequency=0
; MinimumExpectedWaveLaneCount: 0
; MaximumExpectedWaveLaneCount: 4294967295
; UsesViewID: false
; SigInputElements: 1
; SigOutputElements: 1
; SigPatchConstOrPrimElements: 0
; SigInputVectors: 1
; SigOutputVectors[0]: 1
; SigOutputVectors[1]: 0
; SigOutputVectors[2]: 0
; SigOutputVectors[3]: 0
; EntryFunctionName: MainPS
;
;
; Input signature:
;
; Name                 Index             InterpMode DynIdx
; -------------------- ----- ---------------------- ------
; SV_Position              0          noperspective       
;
; Output signature:
;
; Name                 Index             InterpMode DynIdx
; -------------------- ----- ---------------------- ------
; SV_Target                0                              
;
; Buffer Definitions:
;
; cbuffer 
; {
;
;   [96 x i8] (type annotation not present)
;
; }
;
; cbuffer 
; {
;
;   [6044 x i8] (type annotation not present)
;
; }
;
; cbuffer 
; {
;
;   [944 x i8] (type annotation not present)
;
; }
;
; cbuffer 
; {
;
;   [204 x i8] (type annotation not present)
;
; }
;
;
; Resource Bindings:
;
; Name                                 Type  Format         Dim      ID      HLSL Bind  Count
; ------------------------------ ---------- ------- ----------- ------- -------------- ------
;                                   cbuffer      NA          NA     CB0            cb0     1
;                                   cbuffer      NA          NA     CB1            cb1     1
;                                   cbuffer      NA          NA     CB2            cb2     1
;                                   cbuffer      NA          NA     CB3            cb3     1
;                                   sampler      NA          NA      S0             s0     1
;                                   sampler      NA          NA      S1             s1     1
;                                   sampler      NA          NA      S2             s2     1
;                                   texture     f32         buf      T0             t0     1
;                                   texture     f32         buf      T1             t1     1
;                                   texture     f32          2d      T2             t2     1
;                                   texture     f32     2darray      T3             t3     1
;                                   texture     f32          2d      T4             t4     1
;
;
; ViewId state:
;
; Number of inputs: 4, outputs: 4
; Outputs dependent on ViewId: {  }
; Inputs contributing to computation of Outputs:
;   output 0 depends on inputs: { 0, 1 }
;   output 1 depends on inputs: { 0, 1 }
;   output 2 depends on inputs: { 0, 1 }
;   output 3 depends on inputs: { 0, 1 }
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%dx.types.Handle = type { i8* }
%dx.types.ResBind = type { i32, i32, i32, i8 }
%dx.types.ResourceProperties = type { i32, i32 }
%dx.types.CBufRet.f32 = type { float, float, float, float }
%dx.types.CBufRet.i32 = type { i32, i32, i32, i32 }
%dx.types.ResRet.f32 = type { float, float, float, float, i32 }
%"class.Buffer<vector<float, 4> >" = type { <4 x float> }
%"class.Texture2D<vector<float, 4> >" = type { <4 x float>, %"class.Texture2D<vector<float, 4> >::mips_type" }
%"class.Texture2D<vector<float, 4> >::mips_type" = type { i32 }
%"class.Texture2DArray<vector<float, 4> >" = type { <4 x float>, %"class.Texture2DArray<vector<float, 4> >::mips_type" }
%"class.Texture2DArray<vector<float, 4> >::mips_type" = type { i32 }
%"$Globals" = type { <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x i32>, <2 x float> }
%hostlayout.View = type { [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <4 x float>, <4 x float>, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], <4 x float>, <4 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <4 x float>, <4 x float>, <4 x i32>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <2 x float>, <2 x float>, i32, float, float, float, <4 x float>, <4 x float>, <4 x float>, <2 x float>, <2 x float>, float, float, float, float, <3 x float>, float, float, float, float, float, float, float, float, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float, <4 x float>, <3 x float>, float, [2 x <4 x float>], [2 x <4 x float>], <4 x float>, <4 x float>, float, float, float, float, float, float, float, float, float, float, float, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, [2 x <4 x float>], [2 x <4 x float>], [2 x <4 x float>], [2 x <4 x float>], [2 x <4 x float>], <4 x float>, <3 x float>, float, <4 x float>, [4 x <4 x float>], <4 x float>, float, float, float, float, <4 x float>, float, float, float, float, float, float, float, float, <3 x float>, float, float, float, float, float, <4 x float>, float, float, float, float, <4 x float>, float, float, float, float, [8 x <4 x float>], float, float, float, float, i32, float, float, float, <3 x float>, i32, [6 x <4 x float>], [6 x <4 x float>], [6 x <4 x float>], [6 x <4 x float>], float, float, i32, i32, <3 x float>, float, <3 x float>, float, float, float, i32, float, float, float, float, float, float, float, <2 x i32>, float, float, float, float, <3 x float>, float, <3 x float>, float, <2 x float>, <2 x float>, <2 x float>, <2 x float>, <2 x float>, float, float, <3 x float>, float, <2 x float>, <2 x float>, float, float, float, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, <3 x float>, float, float, float, float, float, [2 x <4 x float>], <4 x float>, i32, i32, i32, i32, i32, i32, i32, float, <4 x float>, i32, i32, i32, float, <4 x float>, <4 x float>, <4 x float>, <3 x float>, float, i32, i32, i32, i32, [32 x <4 x i32>], i32, float, float, float, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <2 x float>, float, float, <4 x float>, <4 x float>, <4 x float>, float, float, float, i32, <4 x i32>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, <4 x float>, float, float, i32, i32, i32, i32, i32, i32, <4 x float>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, <3 x i32>, i32, <3 x i32>, i32, <3 x float>, float, <3 x float> }
%MaterialCollection0 = type { [59 x <4 x float>] }
%Material = type { [11 x <4 x float>], i32, i32, i32, i32, i32, i32, i32 }
%struct.SamplerState = type { i32 }

define void @MainPS() {
  %1 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 4, i32 4, i32 0, i8 0 }, i32 4, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %2 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 3, i32 3, i32 0, i8 0 }, i32 3, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %3 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 2, i32 2, i32 0, i8 0 }, i32 2, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %4 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 1, i32 1, i32 0, i8 0 }, i32 1, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %5 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind zeroinitializer, i32 0, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %6 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 2, i32 2, i32 0, i8 3 }, i32 2, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %7 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 1, i32 1, i32 0, i8 3 }, i32 1, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %8 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 0, i32 0, i32 0, i8 3 }, i32 0, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %9 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 3, i32 3, i32 0, i8 2 }, i32 3, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %10 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 2, i32 2, i32 0, i8 2 }, i32 2, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %11 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 1, i32 1, i32 0, i8 2 }, i32 1, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %12 = call %dx.types.Handle @dx.op.createHandleFromBinding(i32 217, %dx.types.ResBind { i32 0, i32 0, i32 0, i8 2 }, i32 0, i1 false)  ; CreateHandleFromBinding(bind,index,nonUniformIndex)
  %13 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %9, %dx.types.ResourceProperties { i32 13, i32 204 })  ; AnnotateHandle(res,props)  resource: CBuffer
  %14 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %10, %dx.types.ResourceProperties { i32 13, i32 944 })  ; AnnotateHandle(res,props)  resource: CBuffer
  %15 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %11, %dx.types.ResourceProperties { i32 13, i32 6044 })  ; AnnotateHandle(res,props)  resource: CBuffer
  %16 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %12, %dx.types.ResourceProperties { i32 13, i32 96 })  ; AnnotateHandle(res,props)  resource: CBuffer
  %17 = call float @dx.op.loadInput.f32(i32 4, i32 0, i32 0, i8 0, i32 undef), !dx.precise !32  ; LoadInput(inputSigId,rowIndex,colIndex,gsVertexAxis)
  %18 = call float @dx.op.loadInput.f32(i32 4, i32 0, i32 0, i8 1, i32 undef), !dx.precise !32  ; LoadInput(inputSigId,rowIndex,colIndex,gsVertexAxis)
  %19 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 16)  ; CBufferLoadLegacy(handle,regIndex)
  %20 = extractvalue %dx.types.CBufRet.f32 %19, 0
  %21 = extractvalue %dx.types.CBufRet.f32 %19, 1
  %22 = extractvalue %dx.types.CBufRet.f32 %19, 2
  %23 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 17)  ; CBufferLoadLegacy(handle,regIndex)
  %24 = extractvalue %dx.types.CBufRet.f32 %23, 0
  %25 = extractvalue %dx.types.CBufRet.f32 %23, 1
  %26 = extractvalue %dx.types.CBufRet.f32 %23, 2
  %27 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 18)  ; CBufferLoadLegacy(handle,regIndex)
  %28 = extractvalue %dx.types.CBufRet.f32 %27, 0
  %29 = extractvalue %dx.types.CBufRet.f32 %27, 1
  %30 = extractvalue %dx.types.CBufRet.f32 %27, 2
  %31 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 31)  ; CBufferLoadLegacy(handle,regIndex)
  %32 = extractvalue %dx.types.CBufRet.f32 %31, 3
  %33 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 73)  ; CBufferLoadLegacy(handle,regIndex)
  %34 = extractvalue %dx.types.CBufRet.f32 %33, 0
  %35 = extractvalue %dx.types.CBufRet.f32 %33, 1
  %36 = extractvalue %dx.types.CBufRet.f32 %33, 2
  %37 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 121)  ; CBufferLoadLegacy(handle,regIndex)
  %38 = extractvalue %dx.types.CBufRet.f32 %37, 0
  %39 = extractvalue %dx.types.CBufRet.f32 %37, 1
  %40 = extractvalue %dx.types.CBufRet.f32 %37, 2
  %41 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 122)  ; CBufferLoadLegacy(handle,regIndex)
  %42 = extractvalue %dx.types.CBufRet.f32 %41, 0
  %43 = extractvalue %dx.types.CBufRet.f32 %41, 1
  %44 = extractvalue %dx.types.CBufRet.f32 %41, 2
  %45 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 182)  ; CBufferLoadLegacy(handle,regIndex)
  %46 = extractvalue %dx.types.CBufRet.f32 %45, 0
  %47 = extractvalue %dx.types.CBufRet.f32 %45, 1
  %48 = extractvalue %dx.types.CBufRet.f32 %45, 2
  %49 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %16, i32 5)  ; CBufferLoadLegacy(handle,regIndex)
  %50 = extractvalue %dx.types.CBufRet.i32 %49, 0
  %51 = extractvalue %dx.types.CBufRet.i32 %49, 1
  %52 = uitofp i32 %50 to float
  %53 = uitofp i32 %51 to float
  %54 = fsub float %17, %52
  %55 = fsub float %18, %53
  %56 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %16, i32 5)  ; CBufferLoadLegacy(handle,regIndex)
  %57 = extractvalue %dx.types.CBufRet.f32 %56, 2
  %58 = extractvalue %dx.types.CBufRet.f32 %56, 3
  %59 = fmul float %57, %54
  %60 = fmul float %58, %55
  %61 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 149)  ; CBufferLoadLegacy(handle,regIndex)
  %62 = extractvalue %dx.types.CBufRet.f32 %61, 0
  %63 = extractvalue %dx.types.CBufRet.f32 %61, 1
  %64 = fmul float %62, %59
  %65 = fmul float %63, %60
  %66 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 148)  ; CBufferLoadLegacy(handle,regIndex)
  %67 = extractvalue %dx.types.CBufRet.f32 %66, 0
  %68 = extractvalue %dx.types.CBufRet.f32 %66, 1
  %69 = fadd float %67, %64
  %70 = fadd float %68, %65
  %71 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 152)  ; CBufferLoadLegacy(handle,regIndex)
  %72 = extractvalue %dx.types.CBufRet.f32 %71, 2
  %73 = extractvalue %dx.types.CBufRet.f32 %71, 3
  %74 = fmul float %72, %69
  %75 = fmul float %73, %70
  %76 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %3, %dx.types.ResourceProperties { i32 2, i32 1033 })  ; AnnotateHandle(res,props)  resource: Texture2D<4xF32>
  %77 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %8, %dx.types.ResourceProperties { i32 14, i32 0 })  ; AnnotateHandle(res,props)  resource: SamplerState
  %78 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %76, %dx.types.Handle %77, float %74, float %75, float undef, float undef, i32 0, i32 0, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %79 = extractvalue %dx.types.ResRet.f32 %78, 0
  %80 = call float @dx.op.binary.f32(i32 35, float %79, float 0x3C32725DE0000000), !dx.precise !32  ; FMax(a,b)
  %81 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 155)  ; CBufferLoadLegacy(handle,regIndex)
  %82 = extractvalue %dx.types.CBufRet.f32 %81, 2
  %83 = fmul float %17, %82
  %84 = fmul float %18, %82
  %85 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 44)  ; CBufferLoadLegacy(handle,regIndex)
  %86 = extractvalue %dx.types.CBufRet.f32 %85, 0
  %87 = extractvalue %dx.types.CBufRet.f32 %85, 1
  %88 = extractvalue %dx.types.CBufRet.f32 %85, 2
  %89 = extractvalue %dx.types.CBufRet.f32 %85, 3
  %90 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 45)  ; CBufferLoadLegacy(handle,regIndex)
  %91 = extractvalue %dx.types.CBufRet.f32 %90, 0
  %92 = extractvalue %dx.types.CBufRet.f32 %90, 1
  %93 = extractvalue %dx.types.CBufRet.f32 %90, 2
  %94 = extractvalue %dx.types.CBufRet.f32 %90, 3
  %95 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 46)  ; CBufferLoadLegacy(handle,regIndex)
  %96 = extractvalue %dx.types.CBufRet.f32 %95, 0
  %97 = extractvalue %dx.types.CBufRet.f32 %95, 1
  %98 = extractvalue %dx.types.CBufRet.f32 %95, 2
  %99 = extractvalue %dx.types.CBufRet.f32 %95, 3
  %100 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 47)  ; CBufferLoadLegacy(handle,regIndex)
  %101 = extractvalue %dx.types.CBufRet.f32 %100, 0
  %102 = extractvalue %dx.types.CBufRet.f32 %100, 1
  %103 = extractvalue %dx.types.CBufRet.f32 %100, 2
  %104 = extractvalue %dx.types.CBufRet.f32 %100, 3
  %105 = fmul float %83, %86
  %106 = call float @dx.op.tertiary.f32(i32 46, float %84, float %91, float %105), !dx.precise !32  ; FMad(a,b,c)
  %107 = call float @dx.op.tertiary.f32(i32 46, float %80, float %96, float %106), !dx.precise !32  ; FMad(a,b,c)
  %108 = call float @dx.op.tertiary.f32(i32 46, float 1.000000e+00, float %101, float %107), !dx.precise !32  ; FMad(a,b,c)
  %109 = fmul float %83, %87
  %110 = call float @dx.op.tertiary.f32(i32 46, float %84, float %92, float %109), !dx.precise !32  ; FMad(a,b,c)
  %111 = call float @dx.op.tertiary.f32(i32 46, float %80, float %97, float %110), !dx.precise !32  ; FMad(a,b,c)
  %112 = call float @dx.op.tertiary.f32(i32 46, float 1.000000e+00, float %102, float %111), !dx.precise !32  ; FMad(a,b,c)
  %113 = fmul float %83, %88
  %114 = call float @dx.op.tertiary.f32(i32 46, float %84, float %93, float %113), !dx.precise !32  ; FMad(a,b,c)
  %115 = call float @dx.op.tertiary.f32(i32 46, float %80, float %98, float %114), !dx.precise !32  ; FMad(a,b,c)
  %116 = call float @dx.op.tertiary.f32(i32 46, float 1.000000e+00, float %103, float %115), !dx.precise !32  ; FMad(a,b,c)
  %117 = fmul float %83, %89
  %118 = call float @dx.op.tertiary.f32(i32 46, float %84, float %94, float %117), !dx.precise !32  ; FMad(a,b,c)
  %119 = call float @dx.op.tertiary.f32(i32 46, float %80, float %99, float %118), !dx.precise !32  ; FMad(a,b,c)
  %120 = call float @dx.op.tertiary.f32(i32 46, float 1.000000e+00, float %104, float %119), !dx.precise !32  ; FMad(a,b,c)
  %121 = fdiv float %108, %120
  %122 = fdiv float %112, %120
  %123 = fdiv float %116, %120
  %124 = fsub float -0.000000e+00, %121
  %125 = fsub float -0.000000e+00, %122
  %126 = fsub float -0.000000e+00, %123
  %127 = call float @dx.op.dot3.f32(i32 55, float %124, float %125, float %126, float %124, float %125, float %126), !dx.precise !32  ; Dot3(ax,ay,az,bx,by,bz)
  %128 = call float @dx.op.unary.f32(i32 25, float %127), !dx.precise !32  ; Rsqrt(value)
  %129 = fmul float %128, %124
  %130 = fmul float %128, %125
  %131 = fmul float %128, %126
  %132 = fsub float -0.000000e+00, %34
  %133 = fsub float -0.000000e+00, %35
  %134 = fsub float -0.000000e+00, %36
  %135 = fcmp oge float %32, 1.000000e+00
  %136 = select i1 %135, float %132, float %129
  %137 = select i1 %135, float %133, float %130
  %138 = select i1 %135, float %134, float %131
  %139 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %16, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %140 = extractvalue %dx.types.CBufRet.f32 %139, 2
  %141 = extractvalue %dx.types.CBufRet.f32 %139, 3
  %142 = fmul fast float %59, %140
  %143 = fmul fast float %60, %141
  %144 = extractvalue %dx.types.CBufRet.f32 %139, 0
  %145 = extractvalue %dx.types.CBufRet.f32 %139, 1
  %146 = fadd fast float %142, %144
  %147 = fadd fast float %143, %145
  %148 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %1, %dx.types.ResourceProperties { i32 2, i32 1033 })  ; AnnotateHandle(res,props)  resource: Texture2D<4xF32>
  %149 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %6, %dx.types.ResourceProperties { i32 14, i32 0 })  ; AnnotateHandle(res,props)  resource: SamplerState
  %150 = call %dx.types.ResRet.f32 @dx.op.sample.f32(i32 60, %dx.types.Handle %148, %dx.types.Handle %149, float %146, float %147, float undef, float undef, i32 0, i32 0, i32 undef, float undef)  ; Sample(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,clamp)
  %151 = extractvalue %dx.types.ResRet.f32 %150, 0
  %152 = extractvalue %dx.types.ResRet.f32 %150, 1
  %153 = extractvalue %dx.types.ResRet.f32 %150, 2
  %154 = extractvalue %dx.types.ResRet.f32 %150, 3
  %155 = call float @dx.op.dot3.f32(i32 55, float %46, float %47, float %48, float %136, float %137, float %138)  ; Dot3(ax,ay,az,bx,by,bz)
  %156 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 0)  ; CBufferLoadLegacy(handle,regIndex)
  %157 = extractvalue %dx.types.CBufRet.f32 %156, 1
  %158 = fmul fast float %157, %155
  %159 = fadd fast float %158, 1.000000e+00
  %160 = fmul fast float %157, %157
  %161 = fsub fast float 1.000000e+00, %160
  %162 = fmul fast float %159, %159
  %163 = fmul fast float %162, 0x402921FB60000000
  %164 = fdiv fast float %161, %163
  %165 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %166 = extractvalue %dx.types.CBufRet.f32 %165, 0
  %167 = extractvalue %dx.types.CBufRet.f32 %165, 1
  %168 = extractvalue %dx.types.CBufRet.f32 %165, 2
  %169 = fmul fast float %166, %164
  %170 = fmul fast float %167, %164
  %171 = fmul fast float %168, %164
  %172 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %173 = extractvalue %dx.types.CBufRet.f32 %172, 0
  %174 = extractvalue %dx.types.CBufRet.f32 %172, 1
  %175 = extractvalue %dx.types.CBufRet.f32 %172, 2
  %176 = fadd fast float %169, %173
  %177 = fadd fast float %170, %174
  %178 = fadd fast float %171, %175
  %179 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %15, i32 78)  ; CBufferLoadLegacy(handle,regIndex)
  %180 = extractvalue %dx.types.CBufRet.f32 %179, 0
  %181 = fmul fast float %180, %79
  %182 = extractvalue %dx.types.CBufRet.f32 %179, 1
  %183 = fadd fast float %181, %182
  %184 = extractvalue %dx.types.CBufRet.f32 %179, 2
  %185 = fmul fast float %184, %79
  %186 = extractvalue %dx.types.CBufRet.f32 %179, 3
  %187 = fsub fast float %185, %186
  %188 = fdiv fast float 1.000000e+00, %187
  %189 = fadd fast float %183, %188
  %190 = extractvalue %dx.types.CBufRet.f32 %172, 3
  %191 = call float @dx.op.binary.f32(i32 36, float %189, float %190)  ; FMin(a,b)
  %192 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 3)  ; CBufferLoadLegacy(handle,regIndex)
  %193 = extractvalue %dx.types.CBufRet.f32 %192, 0
  %194 = extractvalue %dx.types.CBufRet.f32 %192, 1
  %195 = extractvalue %dx.types.CBufRet.f32 %192, 2
  %196 = fmul fast float %191, 0xBFF7154760000000
  %197 = fmul fast float %196, %193
  %198 = fmul fast float %196, %194
  %199 = fmul fast float %196, %195
  %200 = call float @dx.op.unary.f32(i32 21, float %197)  ; Exp(value)
  %201 = call float @dx.op.unary.f32(i32 21, float %198)  ; Exp(value)
  %202 = call float @dx.op.unary.f32(i32 21, float %199)  ; Exp(value)
  %203 = fmul fast float %200, %151
  %204 = fmul fast float %201, %152
  %205 = fmul fast float %202, %153
  %206 = extractvalue %dx.types.CBufRet.f32 %192, 3
  %207 = fmul fast float %196, %206
  %208 = call float @dx.op.unary.f32(i32 21, float %207)  ; Exp(value)
  %209 = fsub fast float %203, %176
  %210 = fsub fast float %204, %177
  %211 = fsub fast float %205, %178
  %212 = fmul fast float %208, %209
  %213 = fmul fast float %208, %210
  %214 = fmul fast float %208, %211
  %215 = fmul float %20, 0.000000e+00
  %216 = call float @dx.op.tertiary.f32(i32 46, float 0.000000e+00, float %24, float %215), !dx.precise !32  ; FMad(a,b,c)
  %217 = call float @dx.op.tertiary.f32(i32 46, float 1.000000e+00, float %28, float %216), !dx.precise !32  ; FMad(a,b,c)
  %218 = fmul float %21, 0.000000e+00
  %219 = call float @dx.op.tertiary.f32(i32 46, float 0.000000e+00, float %25, float %218), !dx.precise !32  ; FMad(a,b,c)
  %220 = call float @dx.op.tertiary.f32(i32 46, float 1.000000e+00, float %29, float %219), !dx.precise !32  ; FMad(a,b,c)
  %221 = fmul float %22, 0.000000e+00
  %222 = call float @dx.op.tertiary.f32(i32 46, float 0.000000e+00, float %26, float %221), !dx.precise !32  ; FMad(a,b,c)
  %223 = call float @dx.op.tertiary.f32(i32 46, float 1.000000e+00, float %30, float %222), !dx.precise !32  ; FMad(a,b,c)
  %224 = call float @dx.op.dot3.f32(i32 55, float %136, float %137, float %138, float %217, float %220, float %223), !dx.precise !32  ; Dot3(ax,ay,az,bx,by,bz)
  %225 = call float @dx.op.unary.f32(i32 6, float %224), !dx.precise !32  ; FAbs(value)
  %226 = fdiv float %136, %225
  %227 = fdiv float %137, %225
  %228 = fdiv float %138, %225
  %229 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 4)  ; CBufferLoadLegacy(handle,regIndex)
  %230 = extractvalue %dx.types.CBufRet.f32 %229, 0
  %231 = fmul float %226, %230
  %232 = fmul float %227, %230
  %233 = fmul float %228, %230
  %234 = fsub float %42, %231
  %235 = fsub float %43, %232
  %236 = fsub float %44, %233
  %237 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %14, i32 1)  ; CBufferLoadLegacy(handle,regIndex)
  %238 = extractvalue %dx.types.CBufRet.f32 %237, 0
  %239 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %14, i32 3)  ; CBufferLoadLegacy(handle,regIndex)
  %240 = extractvalue %dx.types.CBufRet.f32 %239, 0
  %241 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %14, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %242 = extractvalue %dx.types.CBufRet.f32 %241, 3
  %243 = fsub fast float %240, %238
  %244 = fmul fast float %242, %243
  %245 = fadd fast float %244, %238
  %246 = extractvalue %dx.types.CBufRet.f32 %229, 1
  %247 = fptosi float %246 to i32
  %248 = fmul float %38, 2.097152e+06
  %249 = fmul float %39, 2.097152e+06
  %250 = fadd float %248, %234
  %251 = fadd float %249, %235
  %252 = fsub float %250, %248
  %253 = fsub float %251, %249
  %254 = fsub float %234, %252
  %255 = fsub float %235, %253
  %256 = shl nsw i32 %247, 1
  %257 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %5, %dx.types.ResourceProperties { i32 10, i32 1033 })  ; AnnotateHandle(res,props)  resource: TypedBuffer<4xF32>
  %258 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %257, i32 %256, i32 undef)  ; BufferLoad(srv,index,wot)
  %259 = extractvalue %dx.types.ResRet.f32 %258, 1
  %260 = extractvalue %dx.types.ResRet.f32 %258, 2
  %261 = fptosi float %259 to i32
  %262 = fptosi float %260 to i32
  %263 = icmp sgt i32 %262, 0
  br i1 %263, label %264, label %348

; <label>:264                                     ; preds = %0
  br label %265

; <label>:265                                     ; preds = %327, %264
  %266 = phi float [ %340, %327 ], [ 0.000000e+00, %264 ]
  %267 = phi float [ %341, %327 ], [ 0.000000e+00, %264 ]
  %268 = phi float [ %342, %327 ], [ 0.000000e+00, %264 ]
  %269 = phi float [ %343, %327 ], [ 0.000000e+00, %264 ]
  %270 = phi i32 [ %344, %327 ], [ 0, %264 ]
  %271 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %4, %dx.types.ResourceProperties { i32 10, i32 1033 })  ; AnnotateHandle(res,props)  resource: TypedBuffer<4xF32>
  %272 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %271, i32 0, i32 undef)  ; BufferLoad(srv,index,wot)
  %273 = extractvalue %dx.types.ResRet.f32 %272, 1
  %274 = fptosi float %273 to i32
  %275 = add nsw i32 %270, %261
  %276 = shl nsw i32 %275, 1
  %277 = add nsw i32 %274, %276
  %278 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %271, i32 %277, i32 undef)  ; BufferLoad(srv,index,wot)
  %279 = extractvalue %dx.types.ResRet.f32 %278, 0
  %280 = extractvalue %dx.types.ResRet.f32 %278, 1
  %281 = extractvalue %dx.types.ResRet.f32 %278, 2
  %282 = extractvalue %dx.types.ResRet.f32 %278, 3
  %283 = add nsw i32 %277, 1
  %284 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %271, i32 %283, i32 undef)  ; BufferLoad(srv,index,wot)
  %285 = extractvalue %dx.types.ResRet.f32 %284, 0
  %286 = fdiv fast float 0x401921FB60000000, %281
  %287 = fmul fast float %286, %279
  %288 = fmul fast float %286, %280
  %289 = fmul fast float %286, 9.800000e+02
  %290 = call float @dx.op.unary.f32(i32 24, float %289)  ; Sqrt(value)
  %291 = fmul fast float %290, %245
  %292 = fmul float %250, 0x3EA0000000000000
  %293 = fmul float %251, 0x3EA0000000000000
  %294 = call float @dx.op.unary.f32(i32 26, float %292), !dx.precise !32  ; Round_ne(value)
  %295 = call float @dx.op.unary.f32(i32 26, float %293), !dx.precise !32  ; Round_ne(value)
  %296 = call float @dx.op.tertiary.f32(i32 46, float %294, float -2.097152e+06, float %250), !dx.precise !32  ; FMad(a,b,c)
  %297 = call float @dx.op.tertiary.f32(i32 46, float %295, float -2.097152e+06, float %251), !dx.precise !32  ; FMad(a,b,c)
  %298 = fadd float %254, %296
  %299 = fadd float %255, %297
  %300 = call float @dx.op.dot2.f32(i32 54, float %298, float %299, float %287, float %288)  ; Dot2(ax,ay,bx,by)
  %301 = fsub fast float %300, %291
  %302 = call float @dx.op.unary.f32(i32 13, float %301)  ; Sin(value)
  %303 = call float @dx.op.unary.f32(i32 12, float %301)  ; Cos(value)
  %304 = call float @dx.op.unary.f32(i32 6, float %298)  ; FAbs(value)
  %305 = call float @dx.op.unary.f32(i32 6, float %299)  ; FAbs(value)
  %306 = fsub fast float 1.048576e+06, %304
  %307 = fsub fast float 1.048576e+06, %305
  %308 = fcmp fast olt float %306, 4.000000e+02
  %309 = fcmp fast olt float %307, 4.000000e+02
  %310 = or i1 %308, %309
  br i1 %310, label %311, label %327

; <label>:311                                     ; preds = %265
  %312 = fmul fast float %306, 0x3F647AE140000000
  %313 = fmul fast float %307, 0x3F647AE140000000
  %314 = call float @dx.op.unary.f32(i32 7, float %312)  ; Saturate(value)
  %315 = call float @dx.op.unary.f32(i32 7, float %313)  ; Saturate(value)
  %316 = fmul fast float %315, %314
  %317 = call float @dx.op.dot2.f32(i32 54, float %306, float %307, float %287, float %288)  ; Dot2(ax,ay,bx,by)
  %318 = fsub fast float %317, %291
  %319 = call float @dx.op.unary.f32(i32 13, float %318)  ; Sin(value)
  %320 = fsub fast float %302, %319
  %321 = fmul fast float %320, %316
  %322 = fadd fast float %321, %319
  %323 = call float @dx.op.unary.f32(i32 12, float %318)  ; Cos(value)
  %324 = fsub fast float %303, %323
  %325 = fmul fast float %324, %316
  %326 = fadd fast float %325, %323
  br label %327

; <label>:327                                     ; preds = %311, %265
  %328 = phi float [ %322, %311 ], [ %302, %265 ]
  %329 = phi float [ %326, %311 ], [ %303, %265 ]
  %330 = fmul fast float %286, %282
  %331 = fmul fast float %330, %328
  %332 = fmul fast float %331, %279
  %333 = fmul fast float %331, %280
  %334 = fmul fast float %329, %285
  %335 = fmul fast float %282, 5.000000e+01
  %336 = fdiv fast float %335, %281
  %337 = call float @dx.op.unary.f32(i32 7, float %336)  ; Saturate(value)
  %338 = fmul fast float %334, %337
  %339 = fmul fast float %329, %282
  %340 = fadd fast float %332, %266
  %341 = fadd fast float %333, %267
  %342 = fadd fast float %338, %268
  %343 = fadd fast float %339, %269
  %344 = add nuw nsw i32 %270, 1
  %345 = icmp eq i32 %344, %262
  br i1 %345, label %346, label %265

; <label>:346                                     ; preds = %327
  %347 = fmul float %343, 1.000000e+02
  br label %348

; <label>:348                                     ; preds = %346, %0
  %349 = phi float [ 0.000000e+00, %0 ], [ %340, %346 ]
  %350 = phi float [ 0.000000e+00, %0 ], [ %341, %346 ]
  %351 = phi float [ 0.000000e+00, %0 ], [ %342, %346 ]
  %352 = phi float [ 0.000000e+00, %0 ], [ %347, %346 ]
  %353 = fsub fast float 1.000000e+00, %351
  %354 = call float @dx.op.dot3.f32(i32 55, float %349, float %350, float %353, float %349, float %350, float %353)  ; Dot3(ax,ay,az,bx,by,bz)
  %355 = call float @dx.op.unary.f32(i32 25, float %354)  ; Rsqrt(value)
  %356 = fmul fast float %355, %353
  %357 = call float @dx.op.unary.f32(i32 27, float %352)  ; Round_ni(value)
  %358 = fadd fast float %356, 0x3FF028F5C0000000
  %359 = fmul fast float %358, 0x3FDFAEE420000000
  %360 = fadd fast float %359, %357
  %361 = call float @dx.op.unary.f32(i32 27, float %360)  ; Round_ni(value)
  %362 = extractvalue %dx.types.CBufRet.f32 %229, 2
  %363 = fptosi float %362 to i32
  %364 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %4, %dx.types.ResourceProperties { i32 10, i32 1033 })  ; AnnotateHandle(res,props)  resource: TypedBuffer<4xF32>
  %365 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 0, i32 undef)  ; BufferLoad(srv,index,wot)
  %366 = extractvalue %dx.types.ResRet.f32 %365, 0
  %367 = fptosi float %366 to i32
  %368 = shl i32 %363, 1
  %369 = add nsw i32 %367, %368
  %370 = extractvalue %dx.types.ResRet.f32 %365, 3
  %371 = fptosi float %370 to i32
  %372 = or i32 %368, 1
  %373 = add i32 %372, %367
  %374 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 %373, i32 undef)  ; BufferLoad(srv,index,wot)
  %375 = extractvalue %dx.types.ResRet.f32 %374, 1
  %376 = fcmp fast ogt float %375, 0.000000e+00
  %377 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %15, i32 365)  ; CBufferLoadLegacy(handle,regIndex)
  %378 = extractvalue %dx.types.CBufRet.i32 %377, 2
  %379 = select i1 %376, i32 %378, i32 0
  %380 = extractvalue %dx.types.ResRet.f32 %365, 2
  %381 = fptosi float %380 to i32
  %382 = mul nsw i32 %371, %363
  %383 = add nsw i32 %379, %382
  %384 = add nsw i32 %383, %381
  %385 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 %384, i32 undef)  ; BufferLoad(srv,index,wot)
  %386 = extractvalue %dx.types.ResRet.f32 %385, 0
  %387 = extractvalue %dx.types.ResRet.f32 %385, 1
  %388 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 %369, i32 undef)  ; BufferLoad(srv,index,wot)
  %389 = extractvalue %dx.types.ResRet.f32 %388, 0
  %390 = extractvalue %dx.types.ResRet.f32 %388, 1
  %391 = fmul fast float %389, 5.000000e-01
  %392 = fmul fast float %390, 5.000000e-01
  %393 = fsub fast float %234, %386
  %394 = fadd fast float %393, %391
  %395 = fsub fast float %235, %387
  %396 = fadd fast float %395, %392
  %397 = fdiv fast float 1.000000e+00, %389
  %398 = fdiv fast float 1.000000e+00, %390
  %399 = fadd fast float %394, %248
  %400 = fmul fast float %399, %397
  %401 = fadd fast float %396, %249
  %402 = fmul fast float %401, %398
  %403 = call float @dx.op.unary.f32(i32 7, float %400)  ; Saturate(value)
  %404 = call float @dx.op.unary.f32(i32 7, float %402)  ; Saturate(value)
  %405 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 5)  ; CBufferLoadLegacy(handle,regIndex)
  %406 = extractvalue %dx.types.CBufRet.f32 %405, 0
  %407 = extractvalue %dx.types.CBufRet.f32 %405, 1
  %408 = fmul fast float %406, %403
  %409 = fmul fast float %407, %404
  %410 = fadd fast float %408, -5.000000e-01
  %411 = fadd fast float %409, -5.000000e-01
  %412 = call float @dx.op.unary.f32(i32 27, float %410)  ; Round_ni(value)
  %413 = call float @dx.op.unary.f32(i32 27, float %411)  ; Round_ni(value)
  %414 = fadd fast float %412, 5.000000e-01
  %415 = fadd fast float %413, 5.000000e-01
  %416 = fdiv fast float %414, %406
  %417 = fdiv fast float %415, %407
  %418 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %257, i32 0, i32 undef)  ; BufferLoad(srv,index,wot)
  %419 = extractvalue %dx.types.ResRet.f32 %418, 0
  %420 = fptosi float %419 to i32
  %421 = sitofp i32 %420 to float
  %422 = fptosi float %421 to i32
  %423 = shl i32 %422, 1
  %424 = or i32 %423, 1
  %425 = add i32 %424, %367
  %426 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 %425, i32 undef)  ; BufferLoad(srv,index,wot)
  %427 = extractvalue %dx.types.ResRet.f32 %426, 1
  %428 = fcmp fast ogt float %427, 0.000000e+00
  %429 = sitofp i32 %378 to float
  %430 = select i1 %428, float %429, float 0.000000e+00
  %431 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %2, %dx.types.ResourceProperties { i32 7, i32 1033 })  ; AnnotateHandle(res,props)  resource: Texture2DArray<4xF32>
  %432 = call %dx.types.Handle @dx.op.annotateHandle(i32 216, %dx.types.Handle %7, %dx.types.ResourceProperties { i32 14, i32 0 })  ; AnnotateHandle(res,props)  resource: SamplerState
  %433 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %431, %dx.types.Handle %432, float %416, float %417, float %430, float undef, i32 0, i32 0, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %434 = extractvalue %dx.types.ResRet.f32 %433, 0
  %435 = extractvalue %dx.types.ResRet.f32 %433, 1
  %436 = extractvalue %dx.types.ResRet.f32 %433, 2
  %437 = extractvalue %dx.types.ResRet.f32 %433, 3
  %438 = fadd fast float %412, 1.500000e+00
  %439 = fdiv fast float %438, %406
  %440 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %431, %dx.types.Handle %432, float %439, float %417, float %430, float undef, i32 0, i32 0, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %441 = extractvalue %dx.types.ResRet.f32 %440, 0
  %442 = extractvalue %dx.types.ResRet.f32 %440, 1
  %443 = extractvalue %dx.types.ResRet.f32 %440, 2
  %444 = extractvalue %dx.types.ResRet.f32 %440, 3
  %445 = call float @dx.op.unary.f32(i32 22, float %410)  ; Frc(value)
  %446 = call float @dx.op.unary.f32(i32 22, float %411)  ; Frc(value)
  %447 = fsub fast float %441, %434
  %448 = fsub fast float %442, %435
  %449 = fsub fast float %443, %436
  %450 = fsub fast float %444, %437
  %451 = fmul fast float %447, %445
  %452 = fmul fast float %448, %445
  %453 = fmul fast float %449, %445
  %454 = fmul fast float %450, %445
  %455 = fadd fast float %451, %434
  %456 = fadd fast float %452, %435
  %457 = fadd fast float %453, %436
  %458 = fadd fast float %454, %437
  %459 = fadd fast float %413, 1.500000e+00
  %460 = fdiv fast float %459, %407
  %461 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %431, %dx.types.Handle %432, float %416, float %460, float %430, float undef, i32 0, i32 0, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %462 = extractvalue %dx.types.ResRet.f32 %461, 0
  %463 = extractvalue %dx.types.ResRet.f32 %461, 1
  %464 = extractvalue %dx.types.ResRet.f32 %461, 2
  %465 = extractvalue %dx.types.ResRet.f32 %461, 3
  %466 = call %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32 62, %dx.types.Handle %431, %dx.types.Handle %432, float %439, float %460, float %430, float undef, i32 0, i32 0, i32 undef, float 0.000000e+00)  ; SampleLevel(srv,sampler,coord0,coord1,coord2,coord3,offset0,offset1,offset2,LOD)
  %467 = extractvalue %dx.types.ResRet.f32 %466, 0
  %468 = extractvalue %dx.types.ResRet.f32 %466, 1
  %469 = extractvalue %dx.types.ResRet.f32 %466, 2
  %470 = extractvalue %dx.types.ResRet.f32 %466, 3
  %471 = fsub fast float %467, %462
  %472 = fsub fast float %468, %463
  %473 = fsub fast float %469, %464
  %474 = fsub fast float %470, %465
  %475 = fmul fast float %471, %445
  %476 = fmul fast float %472, %445
  %477 = fmul fast float %473, %445
  %478 = fmul fast float %474, %445
  %479 = fsub fast float %462, %455
  %480 = fadd fast float %479, %475
  %481 = fsub fast float %463, %456
  %482 = fadd fast float %481, %476
  %483 = fsub fast float %464, %457
  %484 = fadd fast float %483, %477
  %485 = fsub fast float %465, %458
  %486 = fadd fast float %485, %478
  %487 = fmul fast float %480, %446
  %488 = fmul fast float %482, %446
  %489 = fmul fast float %484, %446
  %490 = fmul fast float %486, %446
  %491 = fadd fast float %489, %457
  %492 = fadd fast float %490, %458
  %493 = add nsw i32 %367, %423
  %494 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 %493, i32 undef)  ; BufferLoad(srv,index,wot)
  %495 = extractvalue %dx.types.ResRet.f32 %494, 2
  %496 = extractvalue %dx.types.ResRet.f32 %494, 3
  %497 = fcmp fast ugt float %491, 0.000000e+00
  %498 = add nsw i32 %493, 1
  br i1 %497, label %499, label %503

; <label>:499                                     ; preds = %348
  %500 = fsub fast float %496, %495
  %501 = fmul fast float %500, %491
  %502 = fadd fast float %501, %495
  br label %510

; <label>:503                                     ; preds = %348
  %504 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 %498, i32 undef)  ; BufferLoad(srv,index,wot)
  %505 = extractvalue %dx.types.ResRet.f32 %504, 0
  %506 = fsub fast float %496, %505
  %507 = fmul fast float %506, %492
  %508 = fadd fast float %505, -2.560000e+02
  %509 = fadd fast float %508, %507
  br label %510

; <label>:510                                     ; preds = %503, %499
  %511 = phi float [ %509, %503 ], [ %502, %499 ]
  %512 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %364, i32 %498, i32 undef)  ; BufferLoad(srv,index,wot)
  %513 = extractvalue %dx.types.ResRet.f32 %512, 0
  %514 = fsub fast float %496, %513
  %515 = fmul fast float %514, %492
  %516 = fsub fast float %511, %513
  %517 = fsub fast float %516, %515
  %518 = call float @dx.op.binary.f32(i32 35, float %517, float 0.000000e+00)  ; FMax(a,b)
  %519 = extractvalue %dx.types.ResRet.f32 %258, 3
  %520 = sitofp i32 %262 to float
  %521 = call float @dx.op.unary.f32(i32 7, float %520)  ; Saturate(value)
  %522 = call float @dx.op.binary.f32(i32 35, float %518, float 0.000000e+00)  ; FMax(a,b)
  %523 = fsub fast float -0.000000e+00, %522
  %524 = fmul fast float %519, 5.000000e-01
  %525 = fdiv fast float %523, %524
  %526 = fmul fast float %525, 0x3FF7154760000000
  %527 = call float @dx.op.unary.f32(i32 21, float %526)  ; Exp(value)
  %528 = fsub fast float 1.000000e+00, %527
  %529 = call float @dx.op.unary.f32(i32 7, float %528)  ; Saturate(value)
  %530 = extractvalue %dx.types.CBufRet.f32 %156, 0
  %531 = fadd fast float %455, -5.000000e-01
  %532 = fadd fast float %531, %487
  %533 = fadd fast float %456, -5.000000e-01
  %534 = fadd fast float %533, %488
  %535 = fmul fast float %532, 2.000000e+00
  %536 = fmul fast float %534, 2.000000e+00
  %537 = fmul fast float %535, %530
  %538 = fmul fast float %536, %530
  %539 = fmul fast float %537, %537
  %540 = fmul fast float %538, %538
  %541 = fadd fast float %539, %540
  %542 = call float @dx.op.unary.f32(i32 24, float %541)  ; Sqrt(value)
  %543 = extractvalue %dx.types.CBufRet.f32 %405, 3
  %544 = fmul fast float %543, %542
  %545 = call float @dx.op.unary.f32(i32 7, float %544)  ; Saturate(value)
  %546 = fsub fast float 1.000000e+00, %545
  %547 = fmul fast float %361, 0x3F847AE140000000
  %548 = fmul fast float %547, %521
  %549 = fmul fast float %548, %529
  %550 = fmul fast float %549, %546
  %551 = fsub fast float %511, %236
  %552 = fadd fast float %551, %550
  %553 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 6)  ; CBufferLoadLegacy(handle,regIndex)
  %554 = extractvalue %dx.types.CBufRet.f32 %553, 0
  %555 = fadd fast float %552, %554
  %556 = extractvalue %dx.types.CBufRet.f32 %553, 1
  %557 = fmul fast float %40, -2.097152e+06
  %558 = fadd fast float %555, %557
  %559 = fmul fast float %558, %556
  %560 = call float @dx.op.unary.f32(i32 7, float %559)  ; Saturate(value)
  %561 = fsub fast float %176, %151
  %562 = fadd fast float %561, %212
  %563 = fsub fast float %177, %152
  %564 = fadd fast float %563, %213
  %565 = fsub fast float %178, %153
  %566 = fadd fast float %565, %214
  %567 = fmul fast float %560, %562
  %568 = fmul fast float %560, %564
  %569 = fmul fast float %560, %566
  %570 = fadd fast float %567, %151
  %571 = fadd fast float %568, %152
  %572 = fadd fast float %569, %153
  %573 = extractvalue %dx.types.CBufRet.f32 %553, 2
  %574 = fmul fast float %573, %552
  %575 = extractvalue %dx.types.CBufRet.f32 %553, 3
  %576 = fsub fast float %575, %574
  %577 = fmul fast float %40, 2.097152e+06
  %578 = fmul fast float %577, %573
  %579 = fadd fast float %576, %578
  %580 = call float @dx.op.unary.f32(i32 7, float %579)  ; Saturate(value)
  %581 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 7)  ; CBufferLoadLegacy(handle,regIndex)
  %582 = extractvalue %dx.types.CBufRet.f32 %581, 0
  %583 = extractvalue %dx.types.CBufRet.f32 %581, 1
  %584 = extractvalue %dx.types.CBufRet.f32 %581, 2
  %585 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 8)  ; CBufferLoadLegacy(handle,regIndex)
  %586 = extractvalue %dx.types.CBufRet.f32 %585, 0
  %587 = extractvalue %dx.types.CBufRet.f32 %585, 1
  %588 = extractvalue %dx.types.CBufRet.f32 %585, 2
  %589 = fsub fast float %582, %586
  %590 = fsub fast float %583, %587
  %591 = fsub fast float %584, %588
  %592 = fmul fast float %589, %580
  %593 = fmul fast float %590, %580
  %594 = fmul fast float %591, %580
  %595 = fadd fast float %592, %586
  %596 = fadd fast float %593, %587
  %597 = fadd fast float %594, %588
  %598 = fmul fast float %595, %164
  %599 = fmul fast float %596, %164
  %600 = fmul fast float %597, %164
  %601 = extractvalue %dx.types.CBufRet.f32 %585, 3
  %602 = fsub fast float %552, %601
  %603 = fmul fast float %602, 0x3EB0000000000000
  %604 = fsub fast float %603, %40
  %605 = fmul fast float %40, %604
  %606 = fmul fast float %602, %602
  %607 = fmul fast float %606, 0x3D50000000000000
  %608 = fsub fast float %607, %605
  %609 = call float @dx.op.unary.f32(i32 24, float %608)  ; Sqrt(value)
  %610 = fmul fast float %217, %217
  %611 = fmul fast float %220, %220
  %612 = fadd fast float %611, %610
  %613 = call float @dx.op.unary.f32(i32 24, float %612)  ; Sqrt(value)
  %614 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 9)  ; CBufferLoadLegacy(handle,regIndex)
  %615 = extractvalue %dx.types.CBufRet.f32 %614, 0
  %616 = fmul fast float %615, %613
  %617 = call float @dx.op.binary.f32(i32 35, float 0x3EE4F8B580000000, float %616)  ; FMax(a,b)
  %618 = fdiv fast float 1.000000e+00, %617
  %619 = extractvalue %dx.types.CBufRet.f32 %614, 1
  %620 = fmul fast float %609, 2.097152e+06
  %621 = fmul fast float %620, %618
  %622 = fmul fast float %621, %619
  %623 = fsub fast float %619, %622
  %624 = call float @dx.op.unary.f32(i32 7, float %623)  ; Saturate(value)
  %625 = extractvalue %dx.types.CBufRet.f32 %614, 2
  %626 = fmul fast float %625, %624
  %627 = fsub fast float %598, %570
  %628 = fsub fast float %599, %571
  %629 = fsub fast float %600, %572
  %630 = fmul fast float %626, %627
  %631 = fmul fast float %626, %628
  %632 = fmul fast float %626, %629
  %633 = fadd fast float %630, %570
  %634 = fadd fast float %631, %571
  %635 = fadd fast float %632, %572
  %636 = extractvalue %dx.types.CBufRet.f32 %614, 3
  %637 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %13, i32 10)  ; CBufferLoadLegacy(handle,regIndex)
  %638 = extractvalue %dx.types.CBufRet.f32 %637, 0
  %639 = extractvalue %dx.types.CBufRet.f32 %637, 1
  %640 = extractvalue %dx.types.CBufRet.f32 %637, 2
  %641 = fsub fast float %638, %633
  %642 = fsub fast float %639, %634
  %643 = fsub fast float %640, %635
  %644 = fmul fast float %641, %636
  %645 = fmul fast float %642, %636
  %646 = fmul fast float %643, %636
  %647 = fadd fast float %644, %633
  %648 = fadd fast float %645, %634
  %649 = fadd fast float %646, %635
  %650 = fadd float %647, 0.000000e+00  ; RenoDX: no clamp, keep negative (wide gamut) channels
  %651 = fadd float %648, 0.000000e+00  ; RenoDX: no clamp, keep negative (wide gamut) channels
  %652 = fadd float %649, 0.000000e+00  ; RenoDX: no clamp, keep negative (wide gamut) channels
  call void @dx.op.storeOutput.f32(i32 5, i32 0, i32 0, i8 0, float %650)  ; StoreOutput(outputSigId,rowIndex,colIndex,value)
  call void @dx.op.storeOutput.f32(i32 5, i32 0, i32 0, i8 1, float %651)  ; StoreOutput(outputSigId,rowIndex,colIndex,value)
  call void @dx.op.storeOutput.f32(i32 5, i32 0, i32 0, i8 2, float %652)  ; StoreOutput(outputSigId,rowIndex,colIndex,value)
  call void @dx.op.storeOutput.f32(i32 5, i32 0, i32 0, i8 3, float %154)  ; StoreOutput(outputSigId,rowIndex,colIndex,value)
  ret void
}

; Function Attrs: nounwind readnone
declare float @dx.op.loadInput.f32(i32, i32, i32, i8, i32) #0

; Function Attrs: nounwind
declare void @dx.op.storeOutput.f32(i32, i32, i32, i8, float) #1

; Function Attrs: nounwind readnone
declare float @dx.op.binary.f32(i32, float, float) #0

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.sampleLevel.f32(i32, %dx.types.Handle, %dx.types.Handle, float, float, float, float, i32, i32, i32, float) #2

; Function Attrs: nounwind readnone
declare float @dx.op.unary.f32(i32, float) #0

; Function Attrs: nounwind readnone
declare float @dx.op.dot3.f32(i32, float, float, float, float, float, float) #0

; Function Attrs: nounwind readnone
declare float @dx.op.tertiary.f32(i32, float, float, float) #0

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32, %dx.types.Handle, i32, i32) #2

; Function Attrs: nounwind readnone
declare float @dx.op.dot2.f32(i32, float, float, float, float) #0

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.sample.f32(i32, %dx.types.Handle, %dx.types.Handle, float, float, float, float, i32, i32, i32, float) #2

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32, %dx.types.Handle, i32) #2

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32, %dx.types.Handle, i32) #2

; Function Attrs: nounwind readnone
declare %dx.types.Handle @dx.op.annotateHandle(i32, %dx.types.Handle, %dx.types.ResourceProperties) #0

; Function Attrs: nounwind readnone
declare %dx.types.Handle @dx.op.createHandleFromBinding(i32, %dx.types.ResBind, i32, i1) #0

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
attributes #2 = { nounwind readonly }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!2}
!dx.shaderModel = !{!3}
!dx.resources = !{!4}
!dx.viewIdState = !{!21}
!dx.entryPoints = !{!22}

!0 = !{!"dxc(private) 1.8.0.0 (private, 00000000)"}
!1 = !{i32 1, i32 6}
!2 = !{i32 1, i32 8}
!3 = !{!"ps", i32 6, i32 6}
!4 = !{!5, null, !12, !17}
!5 = !{!6, !8, !9, !10, !11}
!6 = !{i32 0, %"class.Buffer<vector<float, 4> >"* undef, !"", i32 0, i32 0, i32 1, i32 10, i32 0, !7}
!7 = !{i32 0, i32 9}
!8 = !{i32 1, %"class.Buffer<vector<float, 4> >"* undef, !"", i32 0, i32 1, i32 1, i32 10, i32 0, !7}
!9 = !{i32 2, %"class.Texture2D<vector<float, 4> >"* undef, !"", i32 0, i32 2, i32 1, i32 2, i32 0, !7}
!10 = !{i32 3, %"class.Texture2DArray<vector<float, 4> >"* undef, !"", i32 0, i32 3, i32 1, i32 7, i32 0, !7}
!11 = !{i32 4, %"class.Texture2D<vector<float, 4> >"* undef, !"", i32 0, i32 4, i32 1, i32 2, i32 0, !7}
!12 = !{!13, !14, !15, !16}
!13 = !{i32 0, %"$Globals"* undef, !"", i32 0, i32 0, i32 1, i32 96, null}
!14 = !{i32 1, %hostlayout.View* undef, !"", i32 0, i32 1, i32 1, i32 6044, null}
!15 = !{i32 2, %MaterialCollection0* undef, !"", i32 0, i32 2, i32 1, i32 944, null}
!16 = !{i32 3, %Material* undef, !"", i32 0, i32 3, i32 1, i32 204, null}
!17 = !{!18, !19, !20}
!18 = !{i32 0, %struct.SamplerState* undef, !"", i32 0, i32 0, i32 1, i32 0, null}
!19 = !{i32 1, %struct.SamplerState* undef, !"", i32 0, i32 1, i32 1, i32 0, null}
!20 = !{i32 2, %struct.SamplerState* undef, !"", i32 0, i32 2, i32 1, i32 0, null}
!21 = !{[6 x i32] [i32 4, i32 4, i32 15, i32 15, i32 0, i32 0]}
!22 = !{void ()* @MainPS, !"MainPS", !23, !4, !31}
!23 = !{!24, !28, null}
!24 = !{!25}
!25 = !{i32 0, !"SV_Position", i8 9, i8 3, !26, i8 4, i32 1, i8 4, i32 0, i8 0, !27}
!26 = !{i32 0}
!27 = !{i32 3, i32 3}
!28 = !{!29}
!29 = !{i32 0, !"SV_Target", i8 9, i8 16, !26, i8 0, i32 1, i8 4, i32 0, i8 0, !30}
!30 = !{i32 3, i32 15}
!31 = !{i32 5, !26}
!32 = !{i32 1}
