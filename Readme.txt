/**
 *  opcodeVM code
 *
 *  @E-Mail:yuchao86@gmail.com
 *  @author YuChao
 *  @Copyright under GNU General Public License All rights reserved.
 *  @date 2014-01-21
 *  @see <http://www.gnu.org/licenses/>.
 */

Compilation:
gcc -O2 otcc.c -o otcc -ldl
gcc -O2 otccelf.c -o otccelf 
Self-compilation:
./otccelf otccelf.c otccelf1
