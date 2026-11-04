#include <cassert>
#include "Board.h"

// 置いたブロックが正しく反映されるか
void TestPutAndGetColor();
// ブロックが置かれた状態でリセットするとなくなるか？
void TestReset();
// 同色が３個つながっている状態では消えない
void TestFindEraseThreeDoesNotErase();
// 同色4個が横につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseHorizontal4();
// 消去対象になった同色4個が EraseBlocks() で消えることを確認する
void TestEraseBlocks();
// 同色4個が縦につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseVertical4();
// 同色4個がL字につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseLShape4();
// 同色4個がT字につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseTShape4();
// 同色5個がつながった場合、5個すべてが消去対象になることを確認する
void TestFindEraseFiveBlocks();
// 4マス並んでいても1個が別色なら消去対象にならないことを確認する
void TestFindEraseDifferentColorDoesNotErase();
// 離れた同色ブロックを1つの連結グループとして数えないことを確認する
void TestFindEraseDisconnectedPairsDoNotErase();
// 浮いているブロックを検出できるか
void TestHasFloatingBlocks();
// 1段浮いているブロックが床まで落下し、浮遊状態が解消されることを確認する
void TestDropFloatingBlockToGround();
// 浮いているブロックが DropFloatingBlocks() 1回で1段だけ落ちることを確認する
void TestDropFloatingBlocksOneStep();
// 落下先にブロックがある場合、その直上で停止することを確認する
void TestDropFloatingBlocksStopsAboveBlock();

int main()
{
    TestPutAndGetColor();
    TestReset();
    TestFindEraseThreeDoesNotErase();
    TestFindEraseHorizontal4();
    TestEraseBlocks();
    TestFindEraseVertical4();
    TestFindEraseLShape4();
    TestFindEraseTShape4();
    TestFindEraseFiveBlocks();
    TestFindEraseDifferentColorDoesNotErase();
    TestFindEraseDisconnectedPairsDoNotErase();
    TestHasFloatingBlocks();
    TestDropFloatingBlockToGround();
    TestDropFloatingBlocksOneStep();
    TestDropFloatingBlocksStopsAboveBlock();

    return 0;
}

// 置いたブロックが正しく反映されるか
void TestPutAndGetColor()
{
    Board board;

    board.PutOne(2, 3, BlockColor::Red);

    assert(board.GetColor(2, 3) == BlockColor::Red);
    assert(board.GetColor(0, 0) == BlockColor::Empty);
}

// ブロックが置かれた状態でリセットするとなくなるか？
void TestReset()
{
    Board board;

    board.PutOne(2, 3, BlockColor::Red);
    board.Reset();

    assert(board.GetColor(2, 3) == BlockColor::Empty);
}

// 同色が３個つながっている状態では消えない
void TestFindEraseThreeDoesNotErase()
{
    Board board;

    board.PutOne(3, 0, BlockColor::Red);
    board.PutOne(4, 0, BlockColor::Red);
    board.PutOne(5, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);
}

// 同色4個が横につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseHorizontal4()
{
    Board board;

    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(3, 0, BlockColor::Red);
    board.PutOne(4, 0, BlockColor::Red);
    board.PutOne(5, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);
}

// 消去対象になった同色4個が EraseBlocks() で消えることを確認する
void TestEraseBlocks()
{
    Board board;

    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(3, 0, BlockColor::Red);
    board.PutOne(4, 0, BlockColor::Red);
    board.PutOne(5, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);

    board.EraseBlocks();

    assert(board.GetColor(2, 0) == BlockColor::Empty);
    assert(board.GetColor(3, 0) == BlockColor::Empty);
    assert(board.GetColor(4, 0) == BlockColor::Empty);
    assert(board.GetColor(5, 0) == BlockColor::Empty);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);
}

// 同色4個が縦につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseVertical4()
{
    Board board;

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(0, 1, BlockColor::Red);
    board.PutOne(0, 2, BlockColor::Red);
    board.PutOne(0, 3, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);
}

// 同色4個がL字につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseLShape4()
{
    Board board;

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(1, 0, BlockColor::Red);
    board.PutOne(1, 1, BlockColor::Red);
    board.PutOne(1, 2, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);
}

// 同色4個がT字につながった場合、4個すべてが消去対象になることを確認する
void TestFindEraseTShape4()
{
    Board board;

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(1, 0, BlockColor::Red);
    board.PutOne(1, 1, BlockColor::Red);
    board.PutOne(2, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 4);
}

// 4マス並んでいても1個が別色なら消去対象にならないことを確認する
void TestFindEraseDifferentColorDoesNotErase()
{
    Board board;

    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(3, 0, BlockColor::Blue);
    board.PutOne(4, 0, BlockColor::Red);
    board.PutOne(5, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);
}

// 離れた同色ブロックを1つの連結グループとして数えないことを確認する
void TestFindEraseDisconnectedPairsDoNotErase()
{
    Board board;

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(0, 1, BlockColor::Red);
    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(2, 1, BlockColor::Red);

    assert(board.FindEraseBlocks() == false);
    assert(board.CountEraseBlocks() == 0);
}

// 同色5個がつながった場合、5個すべてが消去対象になることを確認する
void TestFindEraseFiveBlocks()
{
    Board board;

    board.PutOne(0, 0, BlockColor::Red);
    board.PutOne(1, 0, BlockColor::Red);
    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(3, 0, BlockColor::Red);
    board.PutOne(4, 0, BlockColor::Red);

    assert(board.FindEraseBlocks() == true);
    assert(board.CountEraseBlocks() == 5);
}

// 浮いているブロックを検出できるか
void TestHasFloatingBlocks()
{
    Board board;

    assert(board.HasFloatingBlocks() == false);

    board.PutOne(0, 1, BlockColor::Red);

    assert(board.HasFloatingBlocks() == true);
}

// 1段浮いているブロックが床まで落下し、浮遊状態が解消されることを確認する
void TestDropFloatingBlockToGround()
{
    Board board;

    board.PutOne(0, 1, BlockColor::Red);
    board.DropFloatingBlocks();

    assert(board.GetColor(0, 0) == BlockColor::Red);
    assert(board.GetColor(0, 1) == BlockColor::Empty);
    assert(board.HasFloatingBlocks() == false);
}

// 浮いているブロックが DropFloatingBlocks() 1回で1段だけ落ちることを確認する
void TestDropFloatingBlocksOneStep()
{
    Board board;

    board.PutOne(0, 3, BlockColor::Red);

    board.DropFloatingBlocks();

    assert(board.GetColor(0, 3) == BlockColor::Empty);
    assert(board.GetColor(0, 2) == BlockColor::Red);
    assert(board.GetColor(0, 1) == BlockColor::Empty);
    assert(board.HasFloatingBlocks() == true);
}

// 落下先にブロックがある場合、その直上で停止することを確認する
void TestDropFloatingBlocksStopsAboveBlock()
{
    Board board;

    board.PutOne(0, 2, BlockColor::Red);
    board.PutOne(0, 0, BlockColor::Blue);

    board.DropFloatingBlocks();

    assert(board.HasFloatingBlocks() == false);
    assert(board.GetColor(0, 2) == BlockColor::Empty);
    assert(board.GetColor(0, 1) == BlockColor::Red);
    assert(board.GetColor(0, 0) == BlockColor::Blue);
}
