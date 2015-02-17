# -*- coding: utf-8 -*-
"""
Created on Fri Feb 13 16:13:11 2015

functions storing training / testdata combinations to ensure consistent usage
of datasets (same combinations / normalizations / ...)

@author: wirkert
"""

import numpy as np
import monteCarloHelper as mch

def setupTwoDimensionalData(dataFolder):
    trainingParameters   = np.load(dataFolder + "2015February0511:02PMparamters2D.npy")
    trainingReflectances = np.load(dataFolder + "2015February0511:02PMreflectances2D.npy")
    trainingReflectances = mch.normalizeImageQuotient(trainingReflectances, iqBand=4)

    testParameters   = np.load(dataFolder + "2015February1107:43PMparamtersRandom2D.npy")
    testReflectances = np.load(dataFolder + "2015February1107:43PMreflectancesRandom2D.npy")
    testReflectances = mch.normalizeImageQuotient(testReflectances, iqBand=4)

    return trainingParameters, trainingReflectances, testParameters, testReflectances




def setupPerfectArtificialData(dataFolder):
    trainingParameters   = np.load(dataFolder + "2015February1612:04AMparamtersPerfectArtificial.npy")
    trainingParameters   = trainingParameters[:,0:2] # only BVF and Vs for perfect data simulations
    trainingReflectances = np.load(dataFolder + "2015February1612:04AMreflectancesPerfectArtificial1000000photons.npy")
    trainingReflectances = mch.normalizeImageQuotient(trainingReflectances, iqBand=1)

    testParameters   = np.load(dataFolder + "2015February1702:02AMparamtersRandom.npy")
    testParameters   = testParameters[:,0:2] # only BVF and Vs for perfect data simulations
    testReflectances = np.load(dataFolder + "2015February1702:02AMreflectancesRandom.npy")
    testReflectances = mch.normalizeImageQuotient(testReflectances, iqBand=1)

    return trainingParameters, trainingReflectances, testParameters, testReflectances



def setupLogisticRegressionArtificialData(dataFolder):
    uniformReflectances = np.load(dataFolder + "2015February1704:00PMreflectancesRandom1000000photons.npy")
    uniformReflectances = mch.normalizeImageQuotient(uniformReflectances, iqBand=1)

    gaussReflectances = np.load(dataFolder + "2015February1703:56PMreflectancesRandomNonUniform1000000photons.npy")
    gaussReflectances = mch.normalizeImageQuotient(gaussReflectances, iqBand=1)

    return uniformReflectances, gaussReflectances

