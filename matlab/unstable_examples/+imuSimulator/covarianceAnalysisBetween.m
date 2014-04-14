import gtsam.*;

% Test GTSAM covariances on a graph with betweenFactors
% Optionally, you can also enable IMU factors and Camera factors
% Authors: Luca Carlone, David Jensen
% Date: 2014/4/6

clc
clear all
close all

%% Configuration
useRealData = 0;           % controls whether or not to use the Real data (is available) as the ground truth traj
includeIMUFactors = 0;     % if true, IMU type 1 Factors will be generated for the random trajectory
includeCameraFactors = 0;  % not implemented yet
trajectoryLength = 10;     % length of the ground truth trajectory

numMonteCarloRuns = 0;

%% Camera metadata
numberOfLandmarks = 40;    % Total number of visual landmarks, used for camera factors
K = Cal3_S2(500,500,0,640/2,480/2); % Camera calibration
cameraMeasurementNoiseSigma = 1.0;
cameraMeasurementNoise = noiseModel.Isotropic.Sigma(2,cameraMeasurementNoiseSigma);

% Create landmarks
if includeCameraFactors == 1
  for i = 1:numberOfLandmarks
    gtLandmarkPoints(i) = Point3( ...
        [rand()*20*(trajectoryLength*2.0); ...  % uniformly distributed in the x axis along 200% of the trajectory length
         randn()*50; ...   % normally distributed in the y axis with a sigma of 50
         randn()*50]);     % normally distributed in the z axis with a sigma of 50
  end
end

%% Imu metadata
epsBias = 1e-7;
zeroBias = imuBias.ConstantBias(zeros(3,1), zeros(3,1));
IMU_metadata.AccelerometerSigma = 1e-5;
IMU_metadata.GyroscopeSigma = 1e-7;
IMU_metadata.IntegrationSigma = 1e-10;
IMU_metadata.BiasAccelerometerSigma = epsBias;
IMU_metadata.BiasGyroscopeSigma = epsBias;
IMU_metadata.BiasAccOmegaInit = epsBias;
noiseVel =  noiseModel.Isotropic.Sigma(3, 0.1);
noiseBias = noiseModel.Isotropic.Sigma(6, epsBias);

%% Between metadata
if useRealData == 1
  sigma_ang = 1e-1;  sigma_cart = 1;
else
  sigma_ang = 1e-2;  sigma_cart = 0.1;
end
testName = sprintf('sa-%1.2g-sc-%1.2g',sigma_ang,sigma_cart)
folderName = 'results/'

noiseVectorPose = [sigma_ang; sigma_ang; sigma_ang; sigma_cart; sigma_cart; sigma_cart];
noisePose = noiseModel.Diagonal.Sigmas(noiseVectorPose);

%% Create ground truth trajectory
gtValues = Values;
gtGraph = NonlinearFactorGraph;

if useRealData == 1
  subsampleStep = 20;
  
  %% Create a ground truth trajectory from Real data (if available)
  fprintf('\nUsing real data as ground truth\n');
  gtScenario = load('truth_scen2.mat', 'Time', 'Lat', 'Lon', 'Alt', 'Roll', 'Pitch', 'Heading',...
    'VEast', 'VNorth', 'VUp');
  
  Org_lat = gtScenario.Lat(1);
  Org_lon = gtScenario.Lon(1);
  initialPositionECEF = imuSimulator.LatLonHRad_to_ECEF([gtScenario.Lat(1); gtScenario.Lon(1); gtScenario.Alt(1)]);
  
  % Limit the trajectory length
  trajectoryLength = min([length(gtScenario.Lat) trajectoryLength]);
  
  for i=1:trajectoryLength
    currentPoseKey = symbol('x', i-1);
    scenarioInd = subsampleStep * (i-1) + 1
    gtECEF = imuSimulator.LatLonHRad_to_ECEF([gtScenario.Lat(scenarioInd); gtScenario.Lon(scenarioInd); gtScenario.Alt(scenarioInd)]);
    % truth in ENU
    dX = gtECEF(1) - initialPositionECEF(1);
    dY = gtECEF(2) - initialPositionECEF(2);
    dZ = gtECEF(3) - initialPositionECEF(3);
    [xlt, ylt, zlt] = imuSimulator.ct2ENU(dX, dY, dZ,Org_lat, Org_lon);
    
    gtPosition = [xlt, ylt, zlt]';
    gtRotation = Rot3; % Rot3.ypr(gtScenario.Heading(scenarioInd), gtScenario.Pitch(scenarioInd), gtScenario.Roll(scenarioInd));
    currentPose = Pose3(gtRotation, Point3(gtPosition));
    
    % Add values
    gtValues.insert(currentPoseKey, currentPose);
    
    if i==1 % first time step, add priors
      warning('roll-pitch-yaw is different from Rodriguez')
      warning('using identity rotation')
      gtGraph.add(PriorFactorPose3(currentPoseKey, currentPose, noisePose));
      measurements.posePrior = currentPose;
    else
      % Generate measurements as the current pose measured in the frame of
      % the previous pose
      deltaPose = prevPose.between(currentPose);
      measurements.gtDeltaMatrix(i-1,:) = Pose3.Logmap(deltaPose);
      
      % Add the factor to the factor graph
      gtGraph.add(BetweenFactorPose3(currentPoseKey-1, currentPoseKey, deltaPose, noisePose));
    end
    prevPose = currentPose;
  end
else
  %% Create a random trajectory as ground truth
  currentVel = [0; 0; 0];              % initial velocity (used to generate IMU measurements)
  currentPose = Pose3; % initial pose  % initial pose
  deltaT = 0.1;                        % amount of time between IMU measurements
  g = [0; 0; 0];                       % gravity
  omegaCoriolis = [0; 0; 0];           % Coriolis
  
  unsmooth_DP = 0.5; % controls smoothness on translation norm
  unsmooth_DR = 0.1; % controls smoothness on rotation norm
  
  fprintf('\nCreating a random ground truth trajectory\n');
  %% Add priors
  currentPoseKey = symbol('x', 0);
  gtValues.insert(currentPoseKey, currentPose);
  gtGraph.add(PriorFactorPose3(currentPoseKey, currentPose, noisePose));
  
  if includeIMUFactors == 1
    currentVelKey = symbol('v', 0);
    currentBiasKey = symbol('b', 0);
    gtValues.insert(currentVelKey, LieVector(currentVel));
    gtValues.insert(currentBiasKey, zeroBias);
    gtGraph.add(PriorFactorLieVector(currentVelKey, LieVector(currentVel), noiseVel));
    gtGraph.add(PriorFactorConstantBias(currentBiasKey, zeroBias, noiseBias));
  end
  
  if includeCameraFactors == 1
    pointNoiseSigma = 0.1;
    pointPriorNoise  = noiseModel.Isotropic.Sigma(3,pointNoiseSigma);
    gtGraph.add(PriorFactorPoint3(symbol('p',1), gtLandmarkPoints(1), pointPriorNoise));
  end
  
  for i=1:trajectoryLength
    currentPoseKey = symbol('x', i);
    
    gtDeltaPosition = unsmooth_DP*randn(3,1) + [20;0;0]; % create random vector with mean = [20 0 0]
    gtDeltaRotation = unsmooth_DR*randn(3,1) + [0;0;0]; % create random rotation with mean [0 0 0]
    measurements.gtDeltaMatrix(i,:) = [gtDeltaRotation; gtDeltaPosition];
    gtMeasurements.deltaPose = Pose3.Expmap(measurements.gtDeltaMatrix(i,:)');
    
    % "Deduce" ground truth measurements
    % deltaPose are the gt measurements - save them in some structure
    currentPose = currentPose.compose(gtMeasurements.deltaPose);
    gtValues.insert(currentPoseKey, currentPose);
    
    % Add the factors to the factor graph
    gtGraph.add(BetweenFactorPose3(currentPoseKey-1, currentPoseKey, gtMeasurements.deltaPose, noisePose));
    
    %% Add IMU factors
    if includeIMUFactors == 1
      currentVelKey = symbol('v', i);  % not used if includeIMUFactors is false
      currentBiasKey = symbol('b', i); % not used if includeIMUFactors is false
      
      % create accel and gyro measurements based on
      gtMeasurements.imu.gyro = measurements.gtDeltaMatrix(i, 1:3)'./deltaT;
      % acc = (deltaPosition - initialVel * dT) * (2/dt^2)
      gtMeasurements.imu.accel = (measurements.gtDeltaMatrix(i, 4:6)' - currentVel.*deltaT).*(2/(deltaT*deltaT));
      % Initialize preintegration
      imuMeasurement = gtsam.ImuFactorPreintegratedMeasurements(zeroBias, ...
        IMU_metadata.AccelerometerSigma.^2 * eye(3), ...
        IMU_metadata.GyroscopeSigma.^2 * eye(3), ...
        IMU_metadata.IntegrationSigma.^2 * eye(3));
      % Preintegrate
      imuMeasurement.integrateMeasurement(gtMeasurements.imu.accel, gtMeasurements.imu.gyro, deltaT);
      % Add Imu factor
      gtGraph.add(ImuFactor(currentPoseKey-1, currentVelKey-1, currentPoseKey, currentVelKey, ...
        currentBiasKey-1, imuMeasurement, g, omegaCoriolis));
      % Add between on biases
      gtGraph.add(BetweenFactorConstantBias(currentBiasKey-1, currentBiasKey, zeroBias, ...
        noiseModel.Isotropic.Sigma(6, epsBias)));
      % Additional prior on zerobias
      gtGraph.add(PriorFactorConstantBias(currentBiasKey, zeroBias, ...
        noiseModel.Isotropic.Sigma(6, epsBias)));
      
      % update current velocity
      currentVel = measurements.gtDeltaMatrix(i,4:6)'./deltaT;
      gtValues.insert(currentVelKey, LieVector(currentVel));
      
      gtGraph.add(PriorFactorLieVector(currentVelKey, LieVector(currentVel), noiseVel));
      
      gtValues.insert(currentBiasKey, zeroBias);
    end % end of IMU factor creation
    
    %% Add Camera factors
    if includeCameraFactors == 1
      % Create camera with the current pose and calibration K (specified above)
      gtCamera = SimpleCamera(currentPose, K);
      % Project landmarks into the camera
      numSkipped = 0;
      for j = 1:length(gtLandmarkPoints)
        landmarkKey = symbol('p', j);
        try
          Z = gtCamera.project(gtLandmarkPoints(j));
          gtGraph.add(GenericProjectionFactorCal3_S2(Z, cameraMeasurementNoise, currentPoseKey, landmarkKey, K));
        catch
          % Most likely the point is not within the camera's view, which
          % is fine
          numSkipped = numSkipped + 1;
        end
      end
      fprintf('(Pose %d) %d landmarks behind the camera\n', i, numSkipped);
    end % end of Camera factor creation
  end % end of trajectory length
  
  %% Add landmark positions to the Values
  if includeCameraFactors == 1
    for j = 1:length(gtLandmarkPoints)
      landmarkKey = symbol('p', j);
      gtValues.insert(landmarkKey, gtLandmarkPoints(j));
    end
  end
  
end % end of ground truth creation

warning('Additional prior on zerobias')
warning('Additional PriorFactorLieVector on velocities')

% gtPoses = Values;
% for i=0:trajectoryLength
%   currentPoseKey = symbol('x', i);
%   currentPose = gtValues.at(currentPoseKey);
%   gtPoses.insert(currentPoseKey, currentPose);
% end

figure(1)
hold on;
plot3DPoints(gtValues);
plot3DTrajectory(gtValues, '-r', [], 1, Marginals(gtGraph, gtValues));
axis equal

disp('Plotted ground truth')

for k=1:numMonteCarloRuns
  fprintf('Monte Carlo Run %d.\n', k');
  % create a new graph
  graph = NonlinearFactorGraph;
  
  % noisy prior
  currentPoseKey = symbol('x', 0);
  measurements.posePrior = currentPose;
  noisyDelta = noiseVectorPose .* randn(6,1);
  noisyInitialPose = Pose3.Expmap(noisyDelta);
  graph.add(PriorFactorPose3(currentPoseKey, noisyInitialPose, noisePose));
  
  for i=1:size(measurements.gtDeltaMatrix,1)
    currentPoseKey = symbol('x', i);
    
    % for each measurement: add noise and add to graph
    noisyDelta = measurements.gtDeltaMatrix(i,:)' + (noiseVectorPose .* randn(6,1));
    noisyDeltaPose = Pose3.Expmap(noisyDelta);
    
    % Add the factors to the factor graph
    graph.add(BetweenFactorPose3(currentPoseKey-1, currentPoseKey, noisyDeltaPose, noisePose));
  end
  
  % optimize
  optimizer = GaussNewtonOptimizer(graph, gtValues);
  estimate = optimizer.optimize();
  
  figure(1)
  plot3DTrajectory(estimate, '-b');
  
  marginals = Marginals(graph, estimate);
  
  % for each pose in the trajectory
  for i=1:size(measurements.gtDeltaMatrix,1)+1
    % compute estimation errors
    currentPoseKey = symbol('x', i-1);
    gtPosition  = gtValues.at(currentPoseKey).translation.vector;
    estPosition = estimate.at(currentPoseKey).translation.vector;
    estR = estimate.at(currentPoseKey).rotation.matrix;
    errPosition = estPosition - gtPosition;
    
    % compute covariances:
    cov = marginals.marginalCovariance(currentPoseKey);
    covPosition = estR * cov(4:6,4:6) * estR';
    
    % compute NEES using (estimationError = estimatedValues - gtValues) and estimated covariances
    NEES(k,i) = errPosition' * inv(covPosition) * errPosition; % distributed according to a Chi square with n = 3 dof
  end
  
  figure(2)
  hold on
  plot(NEES(k,:),'-b','LineWidth',1.5)
end
%%
ANEES = mean(NEES);
plot(ANEES,'-r','LineWidth',2)
plot(3*ones(size(ANEES,2),1),'k--'); % Expectation(ANEES) = number of dof
box on
set(gca,'Fontsize',16)
title('NEES and ANEES');
%print('-djpeg', horzcat('runs-',testName));
saveas(gcf,horzcat(folderName,'runs-',testName,'.fig'),'fig');

%%
figure(1)
box on
set(gca,'Fontsize',16)
title('Ground truth and estimates for each MC runs');
%print('-djpeg', horzcat('gt-',testName));
saveas(gcf,horzcat(folderName,'gt-',testName,'.fig'),'fig');

%% Let us compute statistics on the overall NEES
n = 3; % position vector dimension
N = numMonteCarloRuns; % number of runs
alpha = 0.01; % confidence level

% mean_value = n*N; % mean value of the Chi-square distribution
% (we divide by n * N and for this reason we expect ANEES around 1)
r1 = chi2inv(alpha, n * N)  / (n * N);
r2 = chi2inv(1-alpha, n * N)  / (n * N);

% output here
fprintf(1, 'r1 = %g\n', r1);
fprintf(1, 'r2 = %g\n', r2);

figure(3)
hold on
plot(ANEES/n,'-b','LineWidth',2)
plot(ones(size(ANEES,2),1),'r-');
plot(r1*ones(size(ANEES,2),1),'k-.');
plot(r2*ones(size(ANEES,2),1),'k-.');
box on
set(gca,'Fontsize',16)
title('NEES normalized by dof VS bounds');
%print('-djpeg', horzcat('ANEES-',testName));
saveas(gcf,horzcat(folderName,'ANEES-',testName,'.fig'),'fig');

logFile = horzcat(folderName,'log-',testName);
save(logFile)

%% NEES COMPUTATION (Bar-Shalom 2001, Section 5.4)
% the nees for a single experiment (i) is defined as
%               NEES_i = xtilda' * inv(P) * xtilda,
% where xtilda in R^n is the estimation
% error, and P is the covariance estimated by the approach we want to test
%
% Average NEES. Given N Monte Carlo simulations, i=1,...,N, the average
% NEES is:
%                   ANEES = sum(NEES_i)/N
% The quantity N*ANEES is distributed according to a Chi-square
% distribution with N*n degrees of freedom.
%
% For the single run case, N=1, therefore NEES = ANEES is distributed
% according to a chi-square distribution with n degrees of freedom (e.g. n=3
% if we are testing a position estimate)
% Therefore its mean should be n (difficult to see from a single run)
% and, with probability alpha, it should hold:
%
% NEES in [r1, r2]
%
% where r1 and r2 are built from the Chi-square distribution

