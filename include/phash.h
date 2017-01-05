/*

    pHash, the open source perceptual hash library
    Copyright (C) 2008-2009 Aetilius, Inc.
    All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Evan Klinger - eklinger@phash.org
    D Grant Starkweather - dstarkweather@phash.org

*/

#ifndef PHASH_H_
#define PHASH_H_

#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <vector>
#include <memory>

#if defined(_WINDOWS) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#  ifndef PHASHAPI
#    define PHASHAPI       _cdecl
#  endif
#  if defined(PHASH_DLL_EXPORT)
#    define PHASHEXPORT    __declspec(dllexport)
#  elif defined(PHASH_DLL_IMPORT)
#    define PHASHEXPORT    __declspec(dllimport)
#  endif
#endif 

#ifndef PHASHAPI
#  define PHASHAPI  
#endif
#ifndef PHASHEXPORT
#  define PHASHEXPORT      extern 
#endif

#define STRINGIZE_(x)      #x
#define STRINGIZE(x)       STRINGIZE_(x)

#define WIDE_(s)           L ## s
#define WIDE(s)            WIDE_(s)

#define CONCATENATE_(a, b) a ## b
#define CONCATENATE(a, b)  CONCATENATE_(a, b)

#if defined(_MSC_VER)
#  define WARNING(msg)      __pragma(message(__FILE__ "("STRINGIZE(__LINE__)") : Warning Msg: "STRINGIZE(msg)))
#elif defined(__GNUC__) || defined(__clang__)
#  define WARNING(msg)     (void)(msg)
#endif

#ifdef _DEBUG
#  include <crtdbg.h>
#  define ASSERT(e)        ( _ASSERT(e) )
#elif !defined(__clang__)
#  define ASSERT(e)        ( __assume(e) )
#else
#  define ASSERT(e)        (void)(e)
#endif

#define NOEXCEPT           noexcept
#define NOEXCEPT_OP(x)     noexcept(x)

#ifndef M_PI
#define M_PI               3.1415926535897932
#endif
#define SQRT_TWO           1.4142135623730950488016887242097

#define ROUNDING_FACTOR(x) (((x) >= 0) ? 0.5 : -0.5) 

/* structure for a single hash */
typedef struct ph_datapoint {
  char* id;
  void* hash;
  float* path;
  uint32_t hash_length;
  uint8_t hash_type;
} DP;

/*
* @brief feature vector info
*/
typedef struct ph_feature_vector {
  double *features;           //the head of the feature array of double's
  int size;                   //the size of the feature array
} Features;

/*
* @brief Digest info
*/
typedef struct ph_digest {
  char *id;                   //hash id
  uint8_t *coeffs;            //the head of the digest integer coefficient array
  int size;                   //the size of the coeff array
} Digest;

typedef struct ph_hash_point {
  uint64_t hash;
  off_t index; /*pos of hash in orig file */
} TxtHashPoint;

typedef struct ph_match {
  off_t first_index;  /* offset into first file */
  off_t second_index; /* offset into second file */
  uint32_t length;    /*length of match between 2 files */
} TxtMatch;

#ifdef __cplusplus
extern "C" {
#endif

/*
* @brief copyright information
*/
const char* ph_about();

/*
* @brief get all the filenames in specified directory
* @param dirname - string value for path and filename
* @param cap - int value for upper limit to number of files
* @param count - int value for number of file names returned
* @return array of pointers to string file names (NULL for error)
**/
char** ph_readfilenames(const char* dirname, int &count);

#ifdef HAVE_PTHREAD

/*
* Allocates path array, does not set id or path
*
* @brief alloc a single data point
*/
PHASHEXPORT DP* ph_malloc_datapoint(int hashtype);

/*
* @brief free a datapoint and its path
*/
PHASHEXPORT void ph_free_datapoint(DP* dp);

#ifdef HAVE_IMAGE_HASH
PHASHEXPORT DP** ph_dct_image_hashes(char* files[], int count, int threads);
#endif

#ifdef HAVE_VIDEO_HASH
PHASHEXPORT DP** ph_dct_video_hashes(char* files[], int count, int threads = 0);
#endif

#endif /* HAVE_PTHREAD */

#ifdef HAVE_IMAGE_HASH

/*
*  Compute the image digest given the file name.
*
* @brief image digest
* @param file - string value for file name of input image.
* @param sigma - double value for the deviation for gaussian filter
* @param gamma - double value for gamma correction on the input image.
* @param digest - Digest struct
* @param N      - int value for number of angles to consider
 */
PHASHEXPORT int ph_image_digest(const char* file, double sigma, double gamma, Digest &digest, int N = 180);

/*
*  Compute the cross correlation of two series vectors
*
* @brief cross correlation for 2 series
* @param x - Digest struct
* @param y - Digest struct
* @param pcc - double value the peak of cross correlation
* @param threshold - double value for the threshold value for which 2 images
*                     are considered the same or different.
* @return - int value - 1 (true) for same, 0 (false) for different, < 0 for error
*/
PHASHEXPORT int ph_crosscorr(const Digest &x, const Digest &y, double &pcc, double threshold = 0.90);

/*
*  Compare 2 images given the file names
*
* @brief compare 2 images
* @param file1 - char string of first image file
* @param file2 - char string of second image file
* @param pcc   - (out) double value for peak of cross correlation
* @param sigma - double value for deviation of gaussian filter
* @param gamma - double value for gamma correction of images
* @param N     - int number for number of angles
* @return int 0 (false) for different image, 1 (true) for same images, less than 0 for error
 */
PHASHEXPORT int ph_compare_images(const char* file1, const char* file2, double &pcc, double sigma = 3.5, double gamma = 1.0, int N = 180, double threshold = 0.90);

/*
* @brief compute dct robust image hash
* @param file string variable for name of file
* @param hash of type uint64_t (must be 64-bit variable)
* @return int value - -1 for failure, 1 for success
 */
PHASHEXPORT int ph_dct_imagehash(const char* file, uint64_t &hash);

/*
* @brief create MH image hash for filename image
* @param filename - string name of image file
* @param N - (out) int value for length of image hash returned
* @param alpha - int scale factor for marr wavelet (default=2)
* @param lvl   - int level of scale factor (default = 1)
* @return uint8_t array
**/
PHASHEXPORT uint8_t* ph_mh_imagehash(const char* filename, int &N, float alpha = 2.0f, float lvl = 1.0f);

/*
* @brief compute hamming distance between two byte arrays
* @param hashA - byte array for first hash
* @param lenA - int length of hashA
* @param hashB - byte array for second hash
* @param lenB - int length of hashB
* @return double value for normalized hamming distance
**/
PHASHEXPORT double ph_mh_hammingdistance(uint8_t* hashA, int lenA, uint8_t* hashB, int lenB);

#endif /* HAVE_IMAGE_HASH */

/*
* Compute video hash based on the dct of normalized video 32x32x64 cube
*
* @brief dct video robust hash
* @param file name of file
* @param hash uint64_t value for hash value
* @return int value - less than 0 for error
*/
PHASHEXPORT int ph_hamming_distance(const uint64_t hash1, const uint64_t hash2);

#ifdef HAVE_VIDEO_HASH

PHASHEXPORT uint64_t* ph_dct_videohash(const char* filename, int &Length);

PHASHEXPORT double ph_dct_videohash_dist(uint64_t* hashA, int N1, uint64_t* hashB, int N2, int threshold = 21);

#endif /* HAVE_VIDEO_HASH */

/*
* @brief textual hash for file
* @param filename - char* name of file
* @param nbpoints - int length of array of return value (out)
* @return TxtHashPoint* array of hash points with respective index into file.
*/
PHASHEXPORT TxtHashPoint* ph_texthash(const char* filename, int* nbpoints);

/*
* @brief compare 2 text hashes
* @param hash1 -TxtHashPoint
* @param N1 - int length of hash1
* @param hash2 - TxtHashPoint
* @param N2 - int length of hash2
* @param nbmatches - int number of matches found (out)
* @return TxtMatch* - list of all matches
*/
PHASHEXPORT TxtMatch* ph_compare_text_hashes(TxtHashPoint* hash1, int N1, TxtHashPoint* hash2, int N2, int* nbmatches);

/* random char mapping for textual hash */
static const uint64_t textkeys[256] = {
    15498727785010036736ULL,
    7275080914684608512ULL,
    14445630958268841984ULL,
    14728618948878663680ULL,
    16816925489502355456ULL,
    3644179549068984320ULL,
    6183768379476672512ULL,
    14171334718745739264ULL,
    5124038997949022208ULL,
    10218941994323935232ULL,
    8806421233143906304ULL,
    11600620999078313984ULL,
    6729085808520724480ULL,
    9470575193177980928ULL,
    17565538031497117696ULL,
    16900815933189128192ULL,
    11726811544871239680ULL,
    13231792875940872192ULL,
    2612106097615437824ULL,
    11196599515807219712ULL,
    300692472869158912ULL,
    4480470094610169856ULL,
    2531475774624497664ULL,
    14834442768343891968ULL,
    2890219059826130944ULL,
    7396118625003765760ULL,
    2394211153875042304ULL,
    2007168123001634816ULL,
    18426904923984625664ULL,
    4026129272715345920ULL,
    9461932602286931968ULL,
    15478888635285110784ULL,
    11301210195989889024ULL,
    5460819486846222336ULL,
    11760763510454222848ULL,
    9671391611782692864ULL,
    9104999035915206656ULL,
    17944531898520829952ULL,
    5395982256818880512ULL,
    14229038033864228864ULL,
    9716729819135213568ULL,
    14202403489962786816ULL,
    7382914959232991232ULL,
    16445815627655938048ULL,
    5226234609431216128ULL,
    6501708925610491904ULL,
    14899887495725449216ULL,
    16953046154302455808ULL,
    1286757727841812480ULL,
    17511993593340887040ULL,
    9702901604990058496ULL,
    1587450200710971392ULL,
    3545719622831439872ULL,
    12234377379614556160ULL,
    16421892977644797952ULL,
    6435938682657570816ULL,
    1183751930908770304ULL,
    369360057810288640ULL,
    8443106805659205632ULL,
    1163912781183844352ULL,
    4395489330525634560ULL,
    17905039407946137600ULL,
    16642801425058889728ULL,
    15696699526515523584ULL,
    4919114829672742912ULL,
    9956820861803560960ULL,
    6921347064588664832ULL,
    14024113865587949568ULL,
    9454608686614839296ULL,
    12317329321407545344ULL,
    9806407834332561408ULL,
    724594440630435840ULL,
    8072988737660780544ULL,
    17189322793565552640ULL,
    17170410068286373888ULL,
    13299223355681931264ULL,
    5244287645466492928ULL,
    13623553490302271488ULL,
    11805525436274835456ULL,
    6531045381898240000ULL,
    12688803018523541504ULL,
    3061682967555342336ULL,
    8118495582609211392ULL,
    16234522641354981376ULL,
    15296060347169898496ULL,
    6093644486544457728ULL,
    4223717250303000576ULL,
    16479812286668603392ULL,
    6463004544354746368ULL,
    12666824055962206208ULL,
    17643725067852447744ULL,
    10858493883470315520ULL,
    12125119390198792192ULL,
    15839782419201785856ULL,
    8108449336276287488ULL,
    17044234219871535104ULL,
    7349859215885729792ULL,
    15029796409454886912ULL,
    12621604020339867648ULL,
    16804467902500569088ULL,
    8900381657152880640ULL,
    3981267780962877440ULL,
    17529062343131004928ULL,
    16973370403403595776ULL,
    2723846500818878464ULL,
    16252728346297761792ULL,
    11825849685375975424ULL,
    7968134154875305984ULL,
    11429537762890481664ULL,
    5184631047941259264ULL,
    14499179536773545984ULL,
    5671596707704471552ULL,
    8246314024086536192ULL,
    4170931045673205760ULL,
    3459375275349901312ULL,
    5095630297546883072ULL,
    10264575540807598080ULL,
    7683092525652901888ULL,
    3128698510505934848ULL,
    16727580085162344448ULL,
    1903172507905556480ULL,
    2325679513238765568ULL,
    9139329894923108352ULL,
    14028291906694283264ULL,
    18165461932440551424ULL,
    17247779239789330432ULL,
    12625782052856266752ULL,
    7068577074616729600ULL,
    13830831575534665728ULL,
    6800641999486582784ULL,
    5426300911997681664ULL,
    4284469158977994752ULL,
    10781909780449460224ULL,
    4508619181419134976ULL,
    2811095488672038912ULL,
    13505756289858273280ULL,
    2314603454007345152ULL,
    14636945174048014336ULL,
    3027146371024027648ULL,
    13744141225487761408ULL,
    1374832156869656576ULL,
    17526325907797573632ULL,
    968993859482681344ULL,
    9621146180956192768ULL,
    3250512879761227776ULL,
    4428369143422517248ULL,
    14716776478503075840ULL,
    13515088420568825856ULL,
    12111461669075419136ULL,
    17845474997598945280ULL,
    11795924440611553280ULL,
    14014634185570910208ULL,
    1724410437128159232ULL,
    2488510261825110016ULL,
    9596182018555641856ULL,
    1443128295859159040ULL,
    1289545427904888832ULL,
    3775219997702356992ULL,
    8511705379065823232ULL,
    15120377003439554560ULL,
    10575862005778874368ULL,
    13938006291063504896ULL,
    958102097297932288ULL,
    2911027712518782976ULL,
    18446625472482639872ULL,
    3769197585969971200ULL,
    16416784002377056256ULL,
    2314484861370368000ULL,
    18406142768607920128ULL,
    997186299691532288ULL,
    16058626086858129408ULL,
    1334230851768025088ULL,
    76768133779554304ULL,
    17027619946340810752ULL,
    10955377032724217856ULL,
    3327281022130716672ULL,
    3009245016053776384ULL,
    7225409437517742080ULL,
    16842369442699542528ULL,
    15120706693719130112ULL,
    6624140361407135744ULL,
    10191549809601544192ULL,
    10688596805580488704ULL,
    8348550798535294976ULL,
    12680060080016588800ULL,
    1838034750426578944ULL,
    9791679102984388608ULL,
    13969605507921477632ULL,
    5613254748128935936ULL,
    18303384482050211840ULL,
    10643238446241415168ULL,
    16189116753907810304ULL,
    13794646699404165120ULL,
    11601340543539347456ULL,
    653400401306976256ULL,
    13794528098177253376ULL,
    15370538129509318656ULL,
    17070184403684032512ULL,
    16109012959547621376ULL,
    15329936824407687168ULL,
    18067370711965499392ULL,
    13720894972696199168ULL,
    16664167676175712256ULL,
    18144138845745053696ULL,
    12301770853917392896ULL,
    9172800635190378496ULL,
    3024675794166218752ULL,
    15311015869971169280ULL,
    16398210081298055168ULL,
    1420301171746144256ULL,
    11984978489980747776ULL,
    4575606368995639296ULL,
    11611850981347688448ULL,
    4226831221851684864ULL,
    12924157176120868864ULL,
    5845166987654725632ULL,
    6064865972278263808ULL,
    4269092205395705856ULL,
    1368028430456586240ULL,
    11678120728997134336ULL,
    4125732613736366080ULL,
    12011266876698001408ULL,
    9420493409195393024ULL,
    17920379313140531200ULL,
    5165863346527797248ULL,
    10073893810502369280ULL,
    13268163337608232960ULL,
    2089657402327564288ULL,
    8697334149066784768ULL,
    10930432232036237312ULL,
    17419594235325186048ULL,
    8317960787322732544ULL,
    6204583131022884864ULL,
    15637017837791346688ULL,
    8015355559358234624ULL,
    59609911230726144ULL,
    6363074407862108160ULL,
    11040031362114387968ULL,
    15370625789791830016ULL,
    4314540415450611712ULL,
    12460332533860532224ULL,
    8908860206063026176ULL,
    8890146784446251008ULL,
    5625439441498669056ULL,
    13135691436504645632ULL,
    3367559886857568256ULL,
    11470606437743329280ULL,
    753813335073357824ULL,
    7636652092253274112ULL,
    12838634868199915520ULL,
    12431934064070492160ULL,
    11762384705989640192ULL,
    6403157671188365312ULL,
    3405683408146268160ULL,
    11236019945420619776ULL,
    11569021017716162560ULL
};

#ifdef __cplusplus
}
#endif

#endif /* PHASH_H_ */
