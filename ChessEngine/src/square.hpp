#pragma once  

#include "piece.hpp"

struct Square  
{
   Color color;
   Piece piece;


   Square(Color c = Color::WHITE, Piece p = Pieces::NONE);
   Piece GetPiece() const;
   void SetPiece(Piece p);
   Color GetColor() const;
   void SetColor(Color c);
   bool IsEmpty() const;
};