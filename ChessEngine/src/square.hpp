#pragma once  

#include "piece.hpp"  

struct Square  
{
   Color color;
   Piece piece;


   Square(Color c = Color::WHITE, Piece p = Pieces::NONE) : color(c), piece(p) {}
   Piece GetPiece() const { return piece; }
   void SetPiece(Piece p) { piece = p; }
   Color GetColor() const { return color; }
   void SetColor(Color c) { color = c; }
   bool IsEmpty() const { return piece == ::Pieces::NONE; }
};