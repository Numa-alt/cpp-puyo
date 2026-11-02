#include <cassert>
#include "Board.h"

int main()
{
    Board board;

    //置いたブロックが正しく反映されるか
    board.PutOne(2, 3, BlockColor::Red);

    assert(board.GetColor(2, 3) == BlockColor::Red);
    assert(board.GetColor(0, 0) == BlockColor::Empty);

    //ブロックが置かれた状態でリセットするとなくなるか？
    board.Reset();

    assert(board.GetColor(2, 3) == BlockColor::Empty);

    //同色が３個つながっている状態では消えない
    board.PutOne(3, 0, BlockColor::Red);
    board.PutOne(4, 0, BlockColor::Red);
    board.PutOne(5, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);

    //同色が4個つながっている状態で消去ブロックが見つかり、消える個数が４か？
    board.Reset();

    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(3, 0, BlockColor::Red);
    board.PutOne(4, 0, BlockColor::Red);
    board.PutOne(5, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);

    //同色が4個つながっている状態で消えるか？
    board.EraseBlocks();

    assert(board.GetColor(2, 0) == BlockColor::Empty);
    assert(board.GetColor(3, 0) == BlockColor::Empty);
    assert(board.GetColor(4, 0) == BlockColor::Empty);
    assert(board.GetColor(5, 0) == BlockColor::Empty);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);

    //４個つながっているが１個だけ別色の場合は消えないか？
    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(3, 0, BlockColor::Blue);
    board.PutOne(4, 0, BlockColor::Red);
    board.PutOne(5, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);

    //縦並びでも消えるか
    board.Reset();

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(0, 1, BlockColor::Red);
    board.PutOne(0, 2, BlockColor::Red);
    board.PutOne(0, 3, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);

    //４つの並びがL字でも消えるか
    board.Reset();

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(1, 0, BlockColor::Red);
    board.PutOne(1, 1, BlockColor::Red);
    board.PutOne(1, 2, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);

    //４つの並びがT字でも消えるか
    board.Reset();

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(1, 0, BlockColor::Red);
    board.PutOne(1, 1, BlockColor::Red);
    board.PutOne(2, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);

    //離れた同色は連結として数えないか
    board.Reset();

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(0, 1, BlockColor::Red);
    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(2, 1, BlockColor::Red);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);

    //5個以上つながっている場合、全て消去対象になるか
    board.Reset();

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(1, 0, BlockColor::Red);
    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(3, 0, BlockColor::Red);
    board.PutOne(4, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 5);

    //浮いているブロックを検出できるか
    board.Reset();

    board.PutOne(0, 1, BlockColor::Red);
    assert(board.HasFloatingBlocks() == true);

    //浮いているブロックが落ちるか？
    board.DropFloatingBlocks();

    assert(board.GetColor(0, 0) == BlockColor::Red);
    assert(board.GetColor(0, 1) == BlockColor::Empty);
    assert(board.HasFloatingBlocks() == false);

    //2段以上浮いているブロックがDropFloatingBlocksで1段だけ落ちるか？
    board.Reset();
    board.PutOne(0, 3, BlockColor::Red);

    board.DropFloatingBlocks();

    assert(board.GetColor(0, 3) == BlockColor::Empty);
    assert(board.GetColor(0, 2) == BlockColor::Red);
    assert(board.GetColor(0, 1) == BlockColor::Empty);
    assert(board.HasFloatingBlocks() == true);

    //浮いたブロックの下にブロックがある場合、そのブロックの上で止まるか
    board.Reset();
    board.PutOne(0, 2, BlockColor::Red);
    board.PutOne(0, 0, BlockColor::Blue);

    board.DropFloatingBlocks();

    assert(board.HasFloatingBlocks() == false);
    assert(board.GetColor(0, 2) == BlockColor::Empty);
    assert(board.GetColor(0, 1) == BlockColor::Red);
    assert(board.GetColor(0, 0) == BlockColor::Blue);


    return 0;
}