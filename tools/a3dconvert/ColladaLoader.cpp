/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ColladaLoader.h"
#include "ColladaConditioner.h"
#include "ColladaGeometry.h"
#include "rsContext.h"
#include "rsFileA3D.h"

#include <dae.h>
#include <dom/domCOLLADA.h>

ColladaLoader::ColladaLoader() {

}

ColladaLoader::~ColladaLoader() {
    clearGeometry();
}

void ColladaLoader::clearGeometry() {
    for (uint32_t i = 0; i < mGeometries.size(); i++) {
        delete mGeometries[i];
    }
    mGeometries.clear();
}

bool ColladaLoader::init(const char *colladaFile) {
    DAE dae;

    clearGeometry();

    bool convertSuceeded = true;

    domCOLLADA* root = dae.open(colladaFile);
    if (!root) {
        fprintf(stderr, "Failed to read file %s.\n", colladaFile);
        return false;
    }

    // We only want to deal with triangulated meshes since rendering complex polygons is not feasible
    ColladaConditioner conditioner;
    conditioner.triangulate(&dae);

    domLibrary_geometries *allGeometry = daeSafeCast<domLibrary_geometries>(root->getDescendant("library_geometries"));

    if (allGeometry) {
        convertSuceeded = convertAllGeometry(allGeometry) && convertSuceeded;
    }

    return convertSuceeded;
}

bool ColladaLoader::convertToA3D(const char *a3dFile) {
    if (mGeometries.size() == 0) {
        return false;
    }
    // Now write all this stuff out
    Context rsc;
    FileA3D file(&rsc);

    for (uint32_t i = 0; i < mGeometries.size(); i++) {
        Mesh *exportedMesh = mGeometries[i]->getMesh(&rsc);
        file.appendToFile(exportedMesh);
        delete exportedMesh;
    }

    file.writeFile(a3dFile);
    return true;
}

bool ColladaLoader::convertAllGeometry(domLibrary_geometries *allGeometry) {

    bool convertSuceeded = true;
    domGeometry_Array &geo_array = allGeometry->getGeometry_array();
    for (size_t i = 0; i < geo_array.getCount(); i++) {
        domGeometry *geometry = geo_array[i];
        const char *geometryName = geometry->getName();
        if (geometryName == NULL) {
            geometryName = geometry->getId();
        }

        domMeshRef mesh = geometry->getMesh();
        if (mesh != NULL) {
            printf("Converting geometry: %s\n", geometryName);
            convertSuceeded = convertGeometry(geometry) && convertSuceeded;
        } else {
            printf("Skipping geometry: %s, unsupported type\n", geometryName);
        }

    }

    return convertSuceeded;
}

bool ColladaLoader::convertGeometry(domGeometry *geometry) {
    bool convertSuceeded = true;

    domMeshRef mesh = geometry->getMesh();

    ColladaGeometry *convertedGeo = new ColladaGeometry();
    convertedGeo->init(geometry);

    mGeometries.push_back(convertedGeo);

    return convertSuceeded;
}
