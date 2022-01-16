/**
 * Header file for the Lyra2 Password Hashing Scheme (PHS).
 * 
 * Author: The Lyra PHC team (http://www.lyra-kdf.net/) -- 2014.
 * 
 * This software is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef LYRA2_H_
#define LYRA2_H_

typedef unsigned char byte ;

//Block length required so Blake2's Initialization Vector (IV) is not overwritten (THIS SHOULD NOT BE MODIFIED)
#define BLOCK_LEN_BLAKE2_SAFE_INT64 8                                   //512 bits (=64 bytes, =8 uint64_t)
#define BLOCK_LEN_BLAKE2_SAFE_BYTES (BLOCK_LEN_BLAKE2_SAFE_INT64 * 8)   //same as above, in bytes


#ifdef BLOCK_LEN_BITS
        #define BLOCK_LEN_INT64 (BLOCK_LEN_BITS/64)      //Block length: 768 bits (=96 bytes, =12 uint64_t)
        #define BLOCK_LEN_BYTES (BLOCK_LEN_BITS/8)       //Block length, in bytes
#else   //default block lenght: 768 bits
        #define BLOCK_LEN_INT64 12                       //Block length: 768 bits (=96 bytes, =12 uint64_t)
        #define BLOCK_LEN_BYTES (BLOCK_LEN_INT64 * 8)    //Block length, in bytes
#endif

#ifndef N_COLS
        #define N_COLS 8                                //Number of columns in the memory matrix: fixed to 64 by default
#endif

#define ROW_LEN_INT64 (BLOCK_LEN_INT64 * N_COLS) //Total length of a row: N_COLS blocks
#define ROW_LEN_BYTES (ROW_LEN_INT64 * 8)        //Number of bytes per row


int LYRA2(void *K, unsigned int kLen, const void *pwd, unsigned int pwdlen, const void *salt, unsigned int saltlen, unsigned int timeCost, unsigned int nRows, unsigned int nCols);

int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen, unsigned int t_cost, unsigned int m_cost);

/////TESTS/////
//void wander(unsigned int timeCost, unsigned int nRows);
//void wander(unsigned int timeCost, unsigned int nRows);
//void wanderc(unsigned int timeCost, unsigned int nRows);
//void wanderd(unsigned int timeCost, unsigned int nRows);
//void setup(unsigned int timeCost, unsigned int nRows);
//void setupv5window(unsigned int timeCost, unsigned int nRows);
//void setupc(unsigned int timeCost, unsigned int nRows);
//void setupd(unsigned int timeCost, unsigned int nRows);
//void setupv5(unsigned int timeCost, unsigned int nRows);
//void setupv5c(unsigned int timeCost, unsigned int nRows);
//void setupv5d(unsigned int timeCost, unsigned int nRows);
///////////////

#endif /* LYRA2_H_ */
