{-# LANGUAGE RankNTypes #-}
{-# LANGUAGE DeriveFunctor #-}

import Numeric.AD
import Numeric.AD.Rank1.Tower (Tower)
import Numeric.SpecFunctions
import Data.List (intercalate)

data Thermistor a = Thermistor { beta, r0, t0 :: a }
                  deriving (Show, Functor)

thermistor :: RealFloat a => Thermistor a
thermistor = Thermistor {beta=3470, t0=273+20, r0=10e3}

temperature :: RealFloat a => Thermistor a -> a -> a
temperature (Thermistor beta r0 t0) r = beta / log (r / rInf)
  where rInf = r0 * exp (-beta / t0)

-- | @taylor' f x@ is the list of coefficients of the Taylor expansion
-- of @f@ around @x@
taylor' :: Fractional a => (forall s. AD s (Tower a) -> AD s (Tower a)) -> a -> [a]
taylor' f x = [d / realToFrac (factorial n) | (n,d) <- zip [0..] (diffs f x)]

-- | @seriesAround x0 coeffs x@ is the value of the power-series around @x0@
-- with coefficients @coeffs@ evaluated at point @x@
seriesAround :: Num a => a -> [a] -> a -> a
seriesAround x0 coeffs x = sum $ zipWith (*) coeffs $ iterate (*x') 1
  where x' = x - x0

generateTaylor = do
    let cs :: [Double]
        cs = take 6
             $ taylor' (temperature thermistor) (r0 thermistor)
    print cs
    let rs = [2000,2002..20000]
    writeFile "temperatures.txt" $ unlines
        $ map (\r->intercalate "\t" [ show r
                                    , show $ temperature thermistor r
                                    , show $ seriesAround (r0 thermistor) cs r
                                    ]) rs

lut =
    -- assumes low-side thermistor
    let res = 12
        npts = 64
        r1 = 10e3 -- high side divider resistor
        codepoints = [0, 2^res / npts .. 2^res]
        rth c = r1 / (2^res/c - 1)
    in map (\c->(c, temperature thermistor $ rth c)) codepoints

generateLUT =
    writeFile "thermistor-lut.c" $ unlines
    $ map (\(c,t)->"  {"++show (round c)++", "++show (round $ 100*t)++"},") lut 

main = generateLUT
