-- DB update 2021_03_14_13 -> 2021_03_16_00
DROP PROCEDURE IF EXISTS `updateDb`;
DELIMITER //
CREATE PROCEDURE updateDb ()
proc:BEGIN DECLARE OK VARCHAR(100) DEFAULT 'FALSE';
SELECT COUNT(*) INTO @COLEXISTS
FROM information_schema.COLUMNS
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'version_db_world' AND COLUMN_NAME = '2021_03_14_13';
IF @COLEXISTS = 0 THEN LEAVE proc; END IF;
START TRANSACTION;
ALTER TABLE version_db_world CHANGE COLUMN 2021_03_14_13 2021_03_16_00 bit;
SELECT sql_rev INTO OK FROM version_db_world WHERE sql_rev = '1614780210274517793'; IF OK <> 'FALSE' THEN LEAVE proc; END IF;
--
-- START UPDATING QUERIES
--

INSERT INTO `version_db_world` (`sql_rev`) VALUES ('1614780210274517793');

-- Rewrite positions for minerals for zone: Bloodmyst Isle
SET
@POOL            = '11646',
@POOLSIZE        = '30',
@POOLDESC        = 'Mineral Veins - Bloodmyst Isle',
@GUID            = '120324,120325,120363,2134521,2134621,2134622,2134623,2134624,2134625,2134626,2134627,2134628,2134629,2134630,2134631,2134632,2134633,2134634,2134635,2134636,2134637,2134638,2134639,2134640,2134641,2134642,2134643,2134644,2134645,2134646,2134647,2134648,2134649,2134650,2134651,2134652,2134653,2134654,2134655,2134656,2134657,2134658,2134659,2134660,2134661,2134662,2134663,2134664,2134665,2134666,2134667,2134668,2134669,2134670,2134671,2134672,2134673,2134674,2134675,2134676,2134677,2134678,2134679,2134680,2134681,2134682,2134683,2134684,2134685,2134686,2134687,2134688,2134689,2134690,2134691,2134692,2134693,2134694,2134695,2134696,2134697,2134698,2134699,2134700,2134701,2134702,2134703,2134704,2134705,2134706,2134707,2134708,2134709,2134710,2134711,2134712,2134713,2134714,2134715,2134716,2134717,2134718,2134719,2134720,2134721,2134722,2134723,2134724,2134725,2134726,2134727,2134728,2134729,2134730,2134731,2134732,2134733,2134734,2134735,2134736,2134737,2134738';

-- Create pool(s)
DELETE FROM `pool_template` WHERE `entry`=@POOL;
INSERT INTO `pool_template` (`entry`,`max_limit`,`description`) VALUES (@POOL,@POOLSIZE,@POOLDESC);

-- Create new gameobjects
DELETE FROM `gameobject` WHERE FIND_IN_SET (`guid`,@GUID);
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`,`ScriptName`,`VerifiedBuild`) VALUES
(120324,1731,530,0,0,1,1,-1488.09,-10655.4,135.281,4.83145,-0,-0,-0.663785,0.747923,300,0,1,'',0),
(120325,1731,530,0,0,1,1,-2620.13,-11774.8,10.9012,3.18007,-0,-0,-0.999815,0.01924,300,0,1,'',0),
(120363,1733,530,0,0,1,1,-1410.64,-11745,24.3925,2.75517,-0,-0,-0.981393,-0.19201,300,0,1,'',0),
(2134521,1731,530,0,0,1,1,-2141.63,-12362,28.6973,6.15893,-0,-0,-0.0620857,0.998071,300,0,1,'',0),
(2134621,1732,530,0,0,1,1,-1312.96,-11484.4,22.1654,1.95674,-0,-0,-0.829588,-0.558376,300,0,1,'',0),
(2134622,1732,530,0,0,1,1,-1378.21,-11353.9,30.4835,3.62241,-0,-0,-0.971241,0.2381,300,0,1,'',0),
(2134623,1731,530,0,0,1,1,-2474.41,-12316.3,15.1395,1.79086,-0,-0,-0.780479,-0.625182,300,0,1,'',0),
(2134624,1731,530,0,0,1,1,-2424.38,-12134.5,35.782,2.10502,-0,-0,-0.86867,-0.495392,300,0,1,'',0),
(2134625,1731,530,0,0,1,1,-1921.96,-11650.8,38.4271,5.58182,-0,-0,-0.343537,0.939139,300,0,1,'',0),
(2134626,1731,530,0,0,1,1,-1341.23,-11605.1,8.95622,5.65439,-0,-0,-0.309242,0.950984,300,0,1,'',0),
(2134627,1731,530,0,0,1,1,-1137.25,-11491.4,-3.61492,4.48289,-0,-0,-0.783418,0.621496,300,0,1,'',0),
(2134628,1732,530,0,0,1,1,-1502.21,-10913.4,60.5297,1.00154,-0,-0,-0.480101,-0.877213,300,0,1,'',0),
(2134629,1732,530,0,0,1,1,-1328.18,-12476.2,30.1035,0.91593,-0,-0,-0.442124,-0.896954,300,0,1,'',0),
(2134630,1732,530,0,0,1,1,-1993.66,-10831.2,71.9193,5.46668,-0,-0,-0.397005,0.917816,300,0,1,'',0),
(2134631,1731,530,0,0,1,1,-2109.01,-10874.3,74.3795,4.78009,-0,-0,-0.682771,0.730633,300,0,1,'',0),
(2134632,1731,530,0,0,1,1,-1663.13,-11742.1,36.28,1.60708,-0,-0,-0.719818,-0.694162,300,0,1,'',0),
(2134633,1731,530,0,0,1,1,-1500.01,-11706.2,35.8202,3.27684,-0,-0,-0.997715,0.0675699,300,0,1,'',0),
(2134634,1731,530,0,0,1,1,-2041.58,-12329.4,12.4268,4.08894,-0,-0,-0.889899,0.456158,300,0,1,'',0),
(2134635,1731,530,0,0,1,1,-1967.73,-11628.9,48.4035,3.25264,-0,-0,-0.998459,0.0554972,300,0,1,'',0),
(2134636,1731,530,0,0,1,1,-1878.46,-11569.2,45.1684,1.02997,-0,-0,-0.492522,-0.8703,300,0,1,'',0),
(2134637,1731,530,0,0,1,1,-2372.18,-11507.2,25.9138,1.38591,-0,-0,-0.638815,-0.769361,300,0,1,'',0),
(2134638,1731,530,0,0,1,1,-2291.71,-11507.2,26.5163,2.19518,-0,-0,-0.890113,-0.45574,300,0,1,'',0),
(2134639,1731,530,0,0,1,1,-2204.71,-12381.6,42.0177,2.9831,-0,-0,-0.996862,-0.0791652,300,0,1,'',0),
(2134640,1731,530,0,0,1,1,-2106.83,-11490.9,64.849,1.25805,-0,-0,-0.588357,-0.808602,300,0,1,'',0),
(2134641,1732,530,0,0,1,1,-2117.71,-10825.4,74.7953,5.46778,-0,-0,-0.3965,0.918035,300,0,1,'',0),
(2134642,1732,530,0,0,1,1,-2250.56,-10920.6,10.7704,5.1742,-0,-0,-0.526512,0.850167,300,0,1,'',0),
(2134643,1731,530,0,0,1,1,-2675.37,-11420.7,27.6011,0.789321,-0,-0,-0.384495,-0.923127,300,0,1,'',0),
(2134644,1732,530,0,0,1,1,-1647.91,-12489.2,-15.8342,4.32738,-0,-0,-0.829326,0.558765,300,0,1,'',0),
(2134645,1731,530,0,0,1,1,-2109.01,-11164.7,72.2675,2.94508,-0,-0,-0.995177,-0.0980966,300,0,1,'',0),
(2134646,1733,530,0,0,1,1,-1428.23,-11520.3,26.1887,4.08894,-0,-0,-0.889899,0.456158,300,0,1,'',0),
(2134647,1731,530,0,0,1,1,-2591.03,-11575.8,27.5438,2.61365,-0,-0,-0.965361,-0.260918,300,0,1,'',0),
(2134648,1731,530,0,0,1,1,-2353.22,-11458.8,25.0549,1.80359,-0,-0,-0.784441,-0.620204,300,0,1,'',0),
(2134649,1731,530,0,0,1,1,-1634.86,-11083.1,70.4121,3.39433,-0,-0,-0.992026,0.126033,300,0,1,'',0),
(2134650,1732,530,0,0,1,1,-1936.6,-10513.1,181.789,1.5631,-0,-0,-0.704379,-0.709824,300,0,1,'',0),
(2134651,1731,530,0,0,1,1,-1895.86,-11406.1,56.617,6.21768,-0,-0,-0.0327457,0.999464,300,0,1,'',0),
(2134652,1731,530,0,0,1,1,-2050.28,-11057,60.3768,0.466683,-0,-0,-0.23123,-0.972899,300,0,1,'',0),
(2134653,1731,530,0,0,1,1,-1541.33,-11471.3,61.391,0.580722,-0,-0,-0.286298,-0.958141,300,0,1,'',0),
(2134654,1731,530,0,0,1,1,-1225.55,-11112.2,-29.8231,0.349187,-0,-0,-0.173708,-0.984797,300,0,1,'',0),
(2134655,1732,530,0,0,1,1,-1815.38,-12026,34.5405,3.98181,-0,-0,-0.913045,0.407859,300,0,1,'',0),
(2134656,1731,530,0,0,1,1,-2286.65,-11245,38.6148,1.15438,-0,-0,-0.54567,-0.838,300,0,1,'',0),
(2134657,1732,530,0,0,1,1,-2150.33,-11291.9,59.6215,0.33882,-0,-0,-0.168601,-0.985685,300,0,1,'',0),
(2134658,1731,530,0,0,1,1,-1965.31,-11014.4,61.4692,1.775,-0,-0,-0.775495,-0.631354,300,0,1,'',0),
(2134659,1731,530,0,0,1,1,-1164.71,-11177.3,-53.5757,4.50788,-0,-0,-0.775591,0.631235,300,0,1,'',0),
(2134660,1732,530,0,0,1,1,-2156.68,-10675.2,49.0752,0.920328,-0,-0,-0.444095,-0.89598,300,0,1,'',0),
(2134661,1731,530,0,0,1,1,-1984.3,-12204.3,20.6867,5.03581,-0,-0,-0.584032,0.811731,300,0,1,'',0),
(2134662,1731,530,0,0,1,1,-1695.76,-10809.1,64.1206,3.56712,-0,-0,-0.977451,0.211162,300,0,1,'',0),
(2134663,1731,530,0,0,1,1,-1565.26,-11190.8,67.9502,1.5587,-0,-0,-0.702817,-0.71137,300,0,1,'',0),
(2134664,1731,530,0,0,1,1,-1811.05,-12126.4,36.0542,4.79862,-0,-0,-0.675971,0.736928,300,0,1,'',0),
(2134665,1731,530,0,0,1,1,-2678.86,-11474.6,27.1757,1.65201,-0,-0,-0.735228,-0.67782,300,0,1,'',0),
(2134666,1733,530,0,0,1,1,-2168.57,-10679.6,47.6714,5.22776,-0,-0,-0.503557,0.863962,300,0,1,'',0),
(2134667,1731,530,0,0,1,1,-1841.48,-11057,67.1545,4.26173,-0,-0,-0.84722,0.531242,300,0,1,'',0),
(2134668,1732,530,0,0,1,1,-2026.36,-10985.2,60.8999,1.73149,-0,-0,-0.761578,-0.648073,300,0,1,'',0),
(2134669,1731,530,0,0,1,1,-1702.28,-12052.1,14.2249,1.61745,-0,-0,-0.723407,-0.690422,300,0,1,'',0),
(2134670,1731,530,0,0,1,1,-2490.87,-11650.9,23.0702,0.565012,-0,-0,-0.278763,-0.96036,300,0,1,'',0),
(2134671,1732,530,0,0,1,1,-1202.03,-12489.2,97.0625,1.31742,-0,-0,-0.612099,-0.790781,300,0,1,'',0),
(2134672,1732,530,0,0,1,1,-1239.01,-11650.8,6.58278,2.34724,-0,-0,-0.922156,-0.386817,300,0,1,'',0),
(2134673,1731,530,0,0,1,1,-1282.51,-12505.6,56.1227,4.43451,-0,-0,-0.798221,0.602365,300,0,1,'',0),
(2134674,1731,530,0,0,1,1,-2176.43,-12287,53.9191,2.47164,-0,-0,-0.944419,-0.328745,300,0,1,'',0),
(2134675,1731,530,0,0,1,1,-1458.68,-11549.6,34.0723,0.33882,-0,-0,-0.168601,-0.985685,300,0,1,'',0),
(2134676,1731,530,0,0,1,1,-1878.46,-11474.6,50.4829,4.37577,-0,-0,-0.815568,0.578661,300,0,1,'',0),
(2134677,1732,530,0,0,1,1,-1204.21,-12420.7,95.9291,0.750054,-0,-0,-0.366298,-0.930498,300,0,1,'',0),
(2134678,1731,530,0,0,1,1,-2207.48,-11116.8,53.2854,6.02824,-0,-0,-0.127126,0.991887,300,0,1,'',0),
(2134679,1732,530,0,0,1,1,-1978.51,-10711.2,118.146,4.55546,-0,-0,-0.760356,0.649507,300,0,1,'',0),
(2134680,1731,530,0,0,1,1,-2572.28,-11213.6,21.2003,0.231691,-0,-0,-0.115587,-0.993297,300,0,1,'',0),
(2134681,1731,530,0,0,1,1,-1174.23,-12542.7,66.8175,0.720042,-0,-0,-0.352294,-0.935889,300,0,1,'',0),
(2134682,1731,530,0,0,1,1,-2287.36,-12322.9,51.7417,1.66583,-0,-0,-0.739895,-0.672722,300,0,1,'',0),
(2134683,1731,530,0,0,1,1,-2615.78,-12189.1,28.3355,0.356099,-0,-0,-0.17711,-0.984191,300,0,1,'',0),
(2134684,1731,530,0,0,1,1,-2539.66,-11915,21.7351,5.82373,-0,-0,-0.227714,0.973728,300,0,1,'',0),
(2134685,1731,530,0,0,1,1,-2150.33,-11647.5,50.494,3.29066,-0,-0,-0.997224,0.0744643,300,0,1,'',0),
(2134686,1733,530,0,0,1,1,-1493.48,-11311.5,72.8389,1.14401,-0,-0,-0.541319,-0.840817,300,0,1,'',0),
(2134687,1733,530,0,0,1,1,-1259.97,-11644.9,5.90562,3.24856,-0,-0,-0.99857,0.0534586,300,0,1,'',0),
(2134688,1732,530,0,0,1,1,-1595.71,-10704.7,135.836,2.87597,-0,-0,-0.991194,-0.132422,300,0,1,'',0),
(2134689,1731,530,0,0,1,1,-2291.67,-11118.2,11.4338,3.49643,-0,-0,-0.984302,0.176491,300,0,1,'',0),
(2134690,1731,530,0,0,1,1,-2030.71,-10760.1,93.8289,4.37577,-0,-0,-0.815568,0.578661,300,0,1,'',0),
(2134691,1731,530,0,0,1,1,-1413.01,-10724.2,80.2255,2.41981,-0,-0,-0.935582,-0.353109,300,0,1,'',0),
(2134692,1732,530,0,0,1,1,-1543.51,-10825.4,65.7066,1.21313,-0,-0,-0.570047,-0.821612,300,0,1,'',0),
(2134693,1731,530,0,0,1,1,-1517.41,-11239.7,68.91,0.580722,-0,-0,-0.286298,-0.958141,300,0,1,'',0),
(2134694,1732,530,0,0,1,1,-1378.21,-10632.9,103.245,3.84704,-0,-0,-0.938436,0.345453,300,0,1,'',0),
(2134695,1731,530,0,0,1,1,-1882.81,-12094.5,28.5107,5.35374,-0,-0,-0.448173,0.893947,300,0,1,'',0),
(2134696,1731,530,0,0,1,1,-1876.28,-11383.3,56.5906,6.1693,-0,-0,-0.0569113,0.998379,300,0,1,'',0),
(2134697,1732,530,0,0,1,1,-1465.21,-11481.1,70.6785,1.49367,-0,-0,-0.679318,-0.733844,300,0,1,'',0),
(2134698,1733,530,0,0,1,1,-1961.35,-10836.5,75.3578,3.59069,-0,-0,-0.974895,0.222664,300,0,1,'',0),
(2134699,1732,530,0,0,1,1,-2002.43,-11340.8,64.6761,3.62587,-0,-0,-0.970828,0.239778,300,0,1,'',0),
(2134700,1731,530,0,0,1,1,-1995.28,-10558.2,180.937,3.5013,-0,-0,-0.98387,0.178888,300,0,1,'',0),
(2134701,1733,530,0,0,1,1,-1292.85,-11483.8,10.8579,2.54705,-0,-0,-0.956139,-0.292915,300,0,1,'',0),
(2134702,1731,530,0,0,1,1,-2543.74,-11213.9,22.2677,3.71069,-0,-0,-0.959788,0.280726,300,0,1,'',0),
(2134703,1732,530,0,0,1,1,-1317.31,-11745.4,13.1934,1.44121,-0,-0,-0.659838,-0.751408,300,0,1,'',0),
(2134704,1731,530,0,0,1,1,-1752.31,-11455,47.9052,0.570355,-0,-0,-0.281328,-0.959612,300,0,1,'',0),
(2134705,1731,530,0,0,1,1,-2206.88,-11109.2,46.7431,6.11401,-0,-0,-0.0844871,0.996425,300,0,1,'',0),
(2134706,1731,530,0,0,1,1,-2334.65,-11213.8,23.1339,5.00015,-0,-0,-0.598412,0.801189,300,0,1,'',0),
(2134707,1731,530,0,0,1,1,-1721.86,-11709.5,42.2004,6.04489,-0,-0,-0.118864,0.992911,300,0,1,'',0),
(2134708,1731,530,0,0,1,1,-2633.48,-10812.1,-17.7529,0.468565,-0,-0,-0.232145,-0.972681,300,0,1,'',0),
(2134709,1732,530,0,0,1,1,-1840.63,-10815.9,72.8682,3.22028,-0,-0,-0.999226,0.0393349,300,0,1,'',0),
(2134710,1732,530,0,0,1,1,-1800.16,-10603.5,150.587,1.78678,-0,-0,-0.779201,-0.626774,300,0,1,'',0),
(2134711,1731,530,0,0,1,1,-2254.73,-10861.3,8.6671,4.66259,-0,-0,-0.724491,0.689284,300,0,1,'',0),
(2134712,1731,530,0,0,1,1,-2111.18,-10906.9,69.0148,3.45308,-0,-0,-0.987897,0.155114,300,0,1,'',0),
(2134713,1731,530,0,0,1,1,-1900.21,-11667.1,42.0804,5.76843,-0,-0,-0.254544,0.967061,300,0,1,'',0),
(2134714,1731,530,0,0,1,1,-2074.21,-11037.4,62.5974,5.24661,-0,-0,-0.495391,0.86867,300,0,1,'',0),
(2134715,1731,530,0,0,1,1,-2287.36,-12052.1,27.5849,0.801891,-0,-0,-0.390289,-0.920693,300,0,1,'',0),
(2134716,1731,530,0,0,1,1,-2263.43,-11778,23.1581,0.349187,-0,-0,-0.173708,-0.984797,300,0,1,'',0),
(2134717,1732,530,0,0,1,1,-1756.66,-12107.5,35.1678,3.85395,-0,-0,-0.937237,0.348694,300,0,1,'',0),
(2134718,1732,530,0,0,1,1,-1051.96,-12551.2,21.4095,5.88593,-0,-0,-0.197324,0.980338,300,0,1,'',0),
(2134719,1732,530,0,0,1,1,-1504.36,-10721,68.8056,4.17282,-0,-0,-0.869991,0.493068,300,0,1,'',0),
(2134720,1731,530,0,0,1,1,-1911.08,-11291.9,66.1291,2.19518,-0,-0,-0.890113,-0.45574,300,0,1,'',0),
(2134721,1731,530,0,0,1,1,-2087.26,-11422.4,65.3849,4.73171,-0,-0,-0.700244,0.713904,300,0,1,'',0),
(2134722,1731,530,0,0,1,1,-1282.51,-11422.4,10.0034,1.89453,-0,-0,-0.811823,-0.583904,300,0,1,'',0),
(2134723,1731,530,0,0,1,1,-2642.91,-11894.6,10.7262,1.82715,-0,-0,-0.791692,-0.610921,300,0,1,'',0),
(2134724,1731,530,0,0,1,1,-1513.06,-11621.4,23.5941,3.63623,-0,-0,-0.969572,0.244807,300,0,1,'',0),
(2134725,1731,530,0,0,1,1,-2045.93,-11239.7,80.7004,5.13258,-0,-0,-0.544091,0.839026,300,0,1,'',0),
(2134726,1731,530,0,0,1,1,-1519.58,-11125.5,79.8109,1.44121,-0,-0,-0.659838,-0.751408,300,0,1,'',0),
(2134727,1732,530,0,0,1,1,-1871.93,-10678.6,111.262,1.61745,-0,-0,-0.723407,-0.690422,300,0,1,'',0),
(2134728,1731,530,0,0,1,1,-2596.21,-11282.1,35.5212,2.87597,-0,-0,-0.991194,-0.132422,300,0,1,'',0),
(2134729,1731,530,0,0,1,1,-2021.07,-10685.3,125.446,3.33323,-0,-0,-0.995413,0.0956708,300,0,1,'',0),
(2134730,1733,530,0,0,1,1,-1353.69,-10657.9,95.6906,5.5994,-0,-0,-0.33527,0.942122,300,0,1,'',0),
(2134731,1731,530,0,0,1,1,-1774.06,-11510.5,48.2173,3.50837,-0,-0,-0.983231,0.182363,300,0,1,'',0),
(2134732,1731,530,0,0,1,1,-1839.31,-10655.7,146.297,2.63752,-0,-0,-0.968407,-0.249376,300,0,1,'',0),
(2134733,1731,530,0,0,1,1,-2544.01,-11419.1,41.3461,2.76884,-0,-0,-0.982682,-0.185299,300,0,1,'',0),
(2134734,1731,530,0,0,1,1,-1193.33,-12084.7,5.45135,5.12566,-0,-0,-0.546987,0.837141,300,0,1,'',0),
(2134735,1731,530,0,0,1,1,-1908.91,-11216.9,58.905,1.27878,-0,-0,-0.596708,-0.802459,300,0,1,'',0),
(2134736,1731,530,0,0,1,1,-2413.51,-11970.5,18.3405,3.39433,-0,-0,-0.992026,0.126033,300,0,1,'',0),
(2134737,1731,530,0,0,1,1,-2106.45,-11498.1,59.5798,5.5369,-0,-0,-0.364544,0.931186,300,0,1,'',0),
(2134738,1733,530,0,0,1,1,-1370.3,-11392.2,26.9523,2.96096,-0,-0,-0.995924,-0.0901952,300,0,1,'',0);

-- Add gameobjects to pools
DELETE FROM `pool_gameobject` WHERE FIND_IN_SET (`guid`,@GUID);
INSERT INTO `pool_gameobject` (`guid`,`pool_entry`,`chance`,`description`) VALUES
(120324,@POOL,0,@POOLDESC),
(120325,@POOL,0,@POOLDESC),
(120363,@POOL,0,@POOLDESC),
(2134521,@POOL,0,@POOLDESC),
(2134621,@POOL,0,@POOLDESC),
(2134622,@POOL,0,@POOLDESC),
(2134623,@POOL,0,@POOLDESC),
(2134624,@POOL,0,@POOLDESC),
(2134625,@POOL,0,@POOLDESC),
(2134626,@POOL,0,@POOLDESC),
(2134627,@POOL,0,@POOLDESC),
(2134628,@POOL,0,@POOLDESC),
(2134629,@POOL,0,@POOLDESC),
(2134630,@POOL,0,@POOLDESC),
(2134631,@POOL,0,@POOLDESC),
(2134632,@POOL,0,@POOLDESC),
(2134633,@POOL,0,@POOLDESC),
(2134634,@POOL,0,@POOLDESC),
(2134635,@POOL,0,@POOLDESC),
(2134636,@POOL,0,@POOLDESC),
(2134637,@POOL,0,@POOLDESC),
(2134638,@POOL,0,@POOLDESC),
(2134639,@POOL,0,@POOLDESC),
(2134640,@POOL,0,@POOLDESC),
(2134641,@POOL,0,@POOLDESC),
(2134642,@POOL,0,@POOLDESC),
(2134643,@POOL,0,@POOLDESC),
(2134644,@POOL,0,@POOLDESC),
(2134645,@POOL,0,@POOLDESC),
(2134646,@POOL,0,@POOLDESC),
(2134647,@POOL,0,@POOLDESC),
(2134648,@POOL,0,@POOLDESC),
(2134649,@POOL,0,@POOLDESC),
(2134650,@POOL,0,@POOLDESC),
(2134651,@POOL,0,@POOLDESC),
(2134652,@POOL,0,@POOLDESC),
(2134653,@POOL,0,@POOLDESC),
(2134654,@POOL,0,@POOLDESC),
(2134655,@POOL,0,@POOLDESC),
(2134656,@POOL,0,@POOLDESC),
(2134657,@POOL,0,@POOLDESC),
(2134658,@POOL,0,@POOLDESC),
(2134659,@POOL,0,@POOLDESC),
(2134660,@POOL,0,@POOLDESC),
(2134661,@POOL,0,@POOLDESC),
(2134662,@POOL,0,@POOLDESC),
(2134663,@POOL,0,@POOLDESC),
(2134664,@POOL,0,@POOLDESC),
(2134665,@POOL,0,@POOLDESC),
(2134666,@POOL,0,@POOLDESC),
(2134667,@POOL,0,@POOLDESC),
(2134668,@POOL,0,@POOLDESC),
(2134669,@POOL,0,@POOLDESC),
(2134670,@POOL,0,@POOLDESC),
(2134671,@POOL,0,@POOLDESC),
(2134672,@POOL,0,@POOLDESC),
(2134673,@POOL,0,@POOLDESC),
(2134674,@POOL,0,@POOLDESC),
(2134675,@POOL,0,@POOLDESC),
(2134676,@POOL,0,@POOLDESC),
(2134677,@POOL,0,@POOLDESC),
(2134678,@POOL,0,@POOLDESC),
(2134679,@POOL,0,@POOLDESC),
(2134680,@POOL,0,@POOLDESC),
(2134681,@POOL,0,@POOLDESC),
(2134682,@POOL,0,@POOLDESC),
(2134683,@POOL,0,@POOLDESC),
(2134684,@POOL,0,@POOLDESC),
(2134685,@POOL,0,@POOLDESC),
(2134686,@POOL,0,@POOLDESC),
(2134687,@POOL,0,@POOLDESC),
(2134688,@POOL,0,@POOLDESC),
(2134689,@POOL,0,@POOLDESC),
(2134690,@POOL,0,@POOLDESC),
(2134691,@POOL,0,@POOLDESC),
(2134692,@POOL,0,@POOLDESC),
(2134693,@POOL,0,@POOLDESC),
(2134694,@POOL,0,@POOLDESC),
(2134695,@POOL,0,@POOLDESC),
(2134696,@POOL,0,@POOLDESC),
(2134697,@POOL,0,@POOLDESC),
(2134698,@POOL,0,@POOLDESC),
(2134699,@POOL,0,@POOLDESC),
(2134700,@POOL,0,@POOLDESC),
(2134701,@POOL,0,@POOLDESC),
(2134702,@POOL,0,@POOLDESC),
(2134703,@POOL,0,@POOLDESC),
(2134704,@POOL,0,@POOLDESC),
(2134705,@POOL,0,@POOLDESC),
(2134706,@POOL,0,@POOLDESC),
(2134707,@POOL,0,@POOLDESC),
(2134708,@POOL,0,@POOLDESC),
(2134709,@POOL,0,@POOLDESC),
(2134710,@POOL,0,@POOLDESC),
(2134711,@POOL,0,@POOLDESC),
(2134712,@POOL,0,@POOLDESC),
(2134713,@POOL,0,@POOLDESC),
(2134714,@POOL,0,@POOLDESC),
(2134715,@POOL,0,@POOLDESC),
(2134716,@POOL,0,@POOLDESC),
(2134717,@POOL,0,@POOLDESC),
(2134718,@POOL,0,@POOLDESC),
(2134719,@POOL,0,@POOLDESC),
(2134720,@POOL,0,@POOLDESC),
(2134721,@POOL,0,@POOLDESC),
(2134722,@POOL,0,@POOLDESC),
(2134723,@POOL,0,@POOLDESC),
(2134724,@POOL,0,@POOLDESC),
(2134725,@POOL,0,@POOLDESC),
(2134726,@POOL,0,@POOLDESC),
(2134727,@POOL,0,@POOLDESC),
(2134728,@POOL,0,@POOLDESC),
(2134729,@POOL,0,@POOLDESC),
(2134730,@POOL,0,@POOLDESC),
(2134731,@POOL,0,@POOLDESC),
(2134732,@POOL,0,@POOLDESC),
(2134733,@POOL,0,@POOLDESC),
(2134734,@POOL,0,@POOLDESC),
(2134735,@POOL,0,@POOLDESC),
(2134736,@POOL,0,@POOLDESC),
(2134737,@POOL,0,@POOLDESC),
(2134738,@POOL,0,@POOLDESC);

-- Respawn rates of gameobjects is 5 minutes
UPDATE `gameobject` SET `spawntimesecs`=300 WHERE FIND_IN_SET (`guid`,@GUID);

--
-- END UPDATING QUERIES
--
COMMIT;
END //
DELIMITER ;
CALL updateDb();
DROP PROCEDURE IF EXISTS `updateDb`;