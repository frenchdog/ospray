// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "common/Util.h"
#include "volume/Volume.h"
#include "transferFunction/TransferFunction.h"
#include "common/Data.h"
#include "Volume_ispc.h"

namespace ospray {

  bool Volume::isDataDistributed() const
  {
    return false;
  }

  std::string Volume::toString() const
  {
    return "ospray::Volume";
  }

  Volume *Volume::createInstance(const std::string &type)
  {
    return createInstanceHelper<Volume, OSP_VOLUME>(type);
  }

  void Volume::commit()
  {
  }

  void Volume::computeSamples(float **results,
                              const vec3f *worldCoordinates,
                              const size_t &count)
  {
    // The ISPC volume container must exist at this point.
    assert(ispcEquivalent != nullptr);

    // Allocate memory for returned volume samples
    *results = (float *)malloc(count * sizeof(float));

    std::vector<float> ispcResults(count);
    float *ptr = ispcResults.data();

    // Compute the sample values.
    ispc::Volume_computeSamples(ispcEquivalent,
                                &ptr,
                                (const ispc::vec3f *)worldCoordinates,
                                count);

    // Copy samples and free ISPC results memory
    memcpy(*results, ptr, count * sizeof(float));
  }

  void Volume::finish()
  {
    // The ISPC volume container must exist at this point.
    assert(ispcEquivalent != nullptr);

    // Make the volume bounding box visible to the application.
    ispc::box3f boundingBox;
    ispc::Volume_getBoundingBox(&boundingBox,ispcEquivalent);
    setParam("boundingBoxMin", boundingBox.lower);
    setParam("boundingBoxMax", boundingBox.upper);
  }

  void Volume::updateEditableParameters()
  {
    // Set the gradient shading flag for the renderer.
    ispc::Volume_setGradientShadingEnabled(ispcEquivalent,
                                           getParam1i("gradientShadingEnabled",
                                                      0));

    ispc::Volume_setPreIntegration(ispcEquivalent,
                                       getParam1i("preIntegration",
                                                  0));

    ispc::Volume_setSingleShade(ispcEquivalent,
                                   getParam1i("singleShade",
                                              1));

    ispc::Volume_setAdaptiveSampling(ispcEquivalent,
                                   getParam1i("adaptiveSampling",
                                              1));

    ispc::Volume_setAdaptiveScalar(ispcEquivalent,
                                 getParam1f("adaptiveScalar", 15.0f));

    ispc::Volume_setAdaptiveMaxSamplingRate(ispcEquivalent,
                                 getParam1f("adaptiveMaxSamplingRate", 2.0f));

    ispc::Volume_setAdaptiveBacktrack(ispcEquivalent,
                                 getParam1f("adaptiveBacktrack", 0.03f));

    // Set the recommended sampling rate for ray casting based renderers.
    ispc::Volume_setSamplingRate(ispcEquivalent,
                                 getParam1f("samplingRate", 0.125f));

    vec3f specular = getParam3f("specular", getParam3f("ks", getParam3f("Ks", vec3f(0.3f))));
    ispc::Volume_setSpecular(ispcEquivalent, (const ispc::vec3f &)specular);
    float Ns = getParam1f("ns", getParam1f("Ns", 20.f));
    ispc::Volume_setNs(ispcEquivalent, Ns);

    // Set the transfer function.
    auto *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error("no transfer function specified on the volume!");

    ispc::Volume_setTransferFunction(ispcEquivalent, transferFunction->getIE());

    // Set the volume clipping box (empty by default for no clipping).
    box3f volumeClippingBox = box3f(getParam3f("volumeClippingBoxLower",
                                               vec3f(0.f)),
                                    getParam3f("volumeClippingBoxUpper",
                                               vec3f(0.f)));
    ispc::Volume_setVolumeClippingBox(ispcEquivalent,
                                      (const ispc::box3f &)volumeClippingBox);

    // Set affine transformation
    AffineSpace3f xfm;
    xfm.l.vx = getParam3f("xfm.l.vx",vec3f(1.f,0.f,0.f));
    xfm.l.vy = getParam3f("xfm.l.vy",vec3f(0.f,1.f,0.f));
    xfm.l.vz = getParam3f("xfm.l.vz",vec3f(0.f,0.f,1.f));
    xfm.p    = getParam3f("xfm.p",   vec3f(0.f,0.f,0.f));
    AffineSpace3f rcp_xfm = rcp(xfm);
    ispc::Volume_setAffineTransformations(ispcEquivalent,
					  (ispc::AffineSpace3f&)xfm,
					  (ispc::AffineSpace3f&)rcp_xfm);
  }

} // ::ospray

