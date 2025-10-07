#pragma once  

#include "piece.hpp"

struct Square  
{
   Piece piece;

   Square(Piece p = Pieces::NONE);
   Piece GetPiece() const;
   void SetPiece(Piece p);
   bool IsEmpty() const;
};