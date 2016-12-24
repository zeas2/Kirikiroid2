/*---------------------------------------------------------------------------*/
/*
	TJS2 Script Engine
	Copyright (C) 2000-2009 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
/*---------------------------------------------------------------------------*/
/*
	Date/time string parser lexical analyzer word cutter.

	This file is always generated from syntax/dp_wordtable.txt by
	syntax/create_word_map.pl. Modification by hand will be lost.

*/

switch(InputPointer[0])
{
case TJS_W('a'):
case TJS_W('A'):
 switch(InputPointer[1])
 {
 case TJS_W('c'):
 case TJS_W('C'):
  switch(InputPointer[2])
  {
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('s'):
   case TJS_W('S'):
    switch(InputPointer[4])
    {
    case TJS_W('t'):
    case TJS_W('T'):
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 1030; return DP_TZ; }
     break;
    }
    break;
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 930; return DP_TZ; }
    break;
   }
   break;
  }
  break;
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -300; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('s'):
   case TJS_W('S'):
    switch(InputPointer[4])
    {
    case TJS_W('t'):
    case TJS_W('T'):
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 1100; return DP_TZ; }
     break;
    }
    break;
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1000; return DP_TZ; }
    break;
   }
   break;
  }
  break;
 case TJS_W('h'):
 case TJS_W('H'):
  switch(InputPointer[2])
  {
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = -1000; return DP_TZ; }
    break;
   }
   break;
  }
  break;
 case TJS_W('m'):
 case TJS_W('M'):
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 0; return DP_AM; }
  break;
 case TJS_W('p'):
 case TJS_W('P'):
  switch(InputPointer[2])
  {
  case TJS_W('r'):
  case TJS_W('R'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 3; return DP_MONTH; }
    break;
   case TJS_W('i'):
   case TJS_W('I'):
    switch(InputPointer[4])
    {
    case TJS_W('l'):
    case TJS_W('L'):
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 3; return DP_MONTH; }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 3; return DP_MONTH; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -400; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('u'):
 case TJS_W('U'):
  switch(InputPointer[2])
  {
  case TJS_W('g'):
  case TJS_W('G'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 7; return DP_MONTH; }
    break;
   case TJS_W('u'):
   case TJS_W('U'):
    switch(InputPointer[4])
    {
    case TJS_W('s'):
    case TJS_W('S'):
     switch(InputPointer[5])
     {
     case TJS_W('t'):
     case TJS_W('T'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 7; return DP_MONTH; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 7; return DP_MONTH; }
   }
   break;
  }
  break;
 case TJS_W('w'):
 case TJS_W('W'):
  switch(InputPointer[2])
  {
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('s'):
   case TJS_W('S'):
    switch(InputPointer[4])
    {
    case TJS_W('t'):
    case TJS_W('T'):
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 900; return DP_TZ; }
     break;
    }
    break;
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 800; return DP_TZ; }
    break;
   }
   break;
  }
  break;
 default:
  if(!TJS_iswalpha(InputPointer[1])) { InputPointer += 1; yylex->val = -100; return DP_TZ; }
 }
 break;
case TJS_W('b'):
case TJS_W('B'):
 switch(InputPointer[1])
 {
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('t'):
 case TJS_W('T'):
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 300; return DP_TZ; }
  break;
 }
 break;
case TJS_W('c'):
case TJS_W('C'):
 switch(InputPointer[1])
 {
 case TJS_W('a'):
 case TJS_W('A'):
  switch(InputPointer[2])
  {
  case TJS_W('d'):
  case TJS_W('D'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1030; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 930; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -1000; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('c'):
 case TJS_W('C'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 800; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -500; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
   switch(InputPointer[3])
   {
   case TJS_W('d'):
   case TJS_W('D'):
    switch(InputPointer[4])
    {
    case TJS_W('s'):
    case TJS_W('S'):
     switch(InputPointer[5])
     {
     case TJS_W('t'):
     case TJS_W('T'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 200; return DP_TZ; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -600; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('d'):
case TJS_W('D'):
 switch(InputPointer[1])
 {
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('c'):
  case TJS_W('C'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 11; return DP_MONTH; }
    break;
   case TJS_W('e'):
   case TJS_W('E'):
    switch(InputPointer[4])
    {
    case TJS_W('m'):
    case TJS_W('M'):
     switch(InputPointer[5])
     {
     case TJS_W('b'):
     case TJS_W('B'):
      switch(InputPointer[6])
      {
      case TJS_W('e'):
      case TJS_W('E'):
       switch(InputPointer[7])
       {
       case TJS_W('r'):
       case TJS_W('R'):
         if(!TJS_iswalpha(InputPointer[8])) { InputPointer += 8; yylex->val = 11; return DP_MONTH; }
        break;
       }
       break;
      }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 11; return DP_MONTH; }
   }
   break;
  }
  break;
 case TJS_W('n'):
 case TJS_W('N'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('e'):
case TJS_W('E'):
 switch(InputPointer[1])
 {
 case TJS_W('a'):
 case TJS_W('A'):
  switch(InputPointer[2])
  {
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1000; return DP_TZ; }
    break;
   }
   break;
  }
  break;
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -400; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
   switch(InputPointer[3])
   {
   case TJS_W('d'):
   case TJS_W('D'):
    switch(InputPointer[4])
    {
    case TJS_W('s'):
    case TJS_W('S'):
     switch(InputPointer[5])
     {
     case TJS_W('t'):
     case TJS_W('T'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 300; return DP_TZ; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 200; return DP_TZ; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -500; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('f'):
case TJS_W('F'):
 switch(InputPointer[1])
 {
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('b'):
  case TJS_W('B'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1; return DP_MONTH; }
    break;
   case TJS_W('r'):
   case TJS_W('R'):
    switch(InputPointer[4])
    {
    case TJS_W('u'):
    case TJS_W('U'):
     switch(InputPointer[5])
     {
     case TJS_W('a'):
     case TJS_W('A'):
      switch(InputPointer[6])
      {
      case TJS_W('r'):
      case TJS_W('R'):
       switch(InputPointer[7])
       {
       case TJS_W('y'):
       case TJS_W('Y'):
         if(!TJS_iswalpha(InputPointer[8])) { InputPointer += 8; yylex->val = 1; return DP_MONTH; }
        break;
       }
       break;
      }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 1; return DP_MONTH; }
   }
   break;
  }
  break;
 case TJS_W('r'):
 case TJS_W('R'):
  switch(InputPointer[2])
  {
  case TJS_W('i'):
  case TJS_W('I'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 5; return DP_WDAY; }
    break;
   case TJS_W('d'):
   case TJS_W('D'):
    switch(InputPointer[4])
    {
    case TJS_W('a'):
    case TJS_W('A'):
     switch(InputPointer[5])
     {
     case TJS_W('y'):
     case TJS_W('Y'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 5; return DP_WDAY; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 5; return DP_WDAY; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('w'):
 case TJS_W('W'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 200; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('g'):
case TJS_W('G'):
 switch(InputPointer[1])
 {
 case TJS_W('m'):
 case TJS_W('M'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 0; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 1000; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('h'):
case TJS_W('H'):
 switch(InputPointer[1])
 {
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -900; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('i'):
case TJS_W('I'):
 switch(InputPointer[1])
 {
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('l'):
  case TJS_W('L'):
   switch(InputPointer[3])
   {
   case TJS_W('e'):
   case TJS_W('E'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1200; return DP_TZ; }
    break;
   case TJS_W('w'):
   case TJS_W('W'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = -1200; return DP_TZ; }
    break;
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 200; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('t'):
 case TJS_W('T'):
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 330; return DP_TZ; }
  break;
 }
 break;
case TJS_W('j'):
case TJS_W('J'):
 switch(InputPointer[1])
 {
 case TJS_W('a'):
 case TJS_W('A'):
  switch(InputPointer[2])
  {
  case TJS_W('n'):
  case TJS_W('N'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 0; return DP_MONTH; }
    break;
   case TJS_W('u'):
   case TJS_W('U'):
    switch(InputPointer[4])
    {
    case TJS_W('a'):
    case TJS_W('A'):
     switch(InputPointer[5])
     {
     case TJS_W('r'):
     case TJS_W('R'):
      switch(InputPointer[6])
      {
      case TJS_W('y'):
      case TJS_W('Y'):
        if(!TJS_iswalpha(InputPointer[7])) { InputPointer += 7; yylex->val = 0; return DP_MONTH; }
       break;
      }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 0; return DP_MONTH; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 900; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('t'):
 case TJS_W('T'):
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 730; return DP_TZ; }
  break;
 case TJS_W('u'):
 case TJS_W('U'):
  switch(InputPointer[2])
  {
  case TJS_W('.'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 5; return DP_MONTH; }
   break;
  case TJS_W('l'):
  case TJS_W('L'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 6; return DP_MONTH; }
    break;
   case TJS_W('y'):
   case TJS_W('Y'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 6; return DP_MONTH; }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 6; return DP_MONTH; }
   }
   break;
  case TJS_W('n'):
  case TJS_W('N'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 5; return DP_MONTH; }
    break;
   case TJS_W('e'):
   case TJS_W('E'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 5; return DP_MONTH; }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 5; return DP_MONTH; }
   }
   break;
  default:
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 5; return DP_MONTH; }
  }
  break;
 }
 break;
case TJS_W('k'):
case TJS_W('K'):
 switch(InputPointer[1])
 {
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 900; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('l'):
case TJS_W('L'):
 switch(InputPointer[1])
 {
 case TJS_W('i'):
 case TJS_W('I'):
  switch(InputPointer[2])
  {
  case TJS_W('g'):
  case TJS_W('G'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1000; return DP_TZ; }
    break;
   }
   break;
  }
  break;
 }
 break;
case TJS_W('m'):
case TJS_W('M'):
 switch(InputPointer[1])
 {
 case TJS_W('a'):
 case TJS_W('A'):
  switch(InputPointer[2])
  {
  case TJS_W('r'):
  case TJS_W('R'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 2; return DP_MONTH; }
    break;
   case TJS_W('c'):
   case TJS_W('C'):
    switch(InputPointer[4])
    {
    case TJS_W('h'):
    case TJS_W('H'):
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 2; return DP_MONTH; }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 2; return DP_MONTH; }
   }
   break;
  case TJS_W('y'):
  case TJS_W('Y'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 4; return DP_MONTH; }
   break;
  }
  break;
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -600; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 200; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('t'):
  case TJS_W('T'):
   switch(InputPointer[3])
   {
   case TJS_W('d'):
   case TJS_W('D'):
    switch(InputPointer[4])
    {
    case TJS_W('s'):
    case TJS_W('S'):
     switch(InputPointer[5])
     {
     case TJS_W('t'):
     case TJS_W('T'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 200; return DP_TZ; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   }
   break;
  case TJS_W('w'):
  case TJS_W('W'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 100; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('z'):
  case TJS_W('Z'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('o'):
 case TJS_W('O'):
  switch(InputPointer[2])
  {
  case TJS_W('n'):
  case TJS_W('N'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1; return DP_WDAY; }
    break;
   case TJS_W('d'):
   case TJS_W('D'):
    switch(InputPointer[4])
    {
    case TJS_W('a'):
    case TJS_W('A'):
     switch(InputPointer[5])
     {
     case TJS_W('y'):
     case TJS_W('Y'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 1; return DP_WDAY; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 1; return DP_WDAY; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -700; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('t'):
 case TJS_W('T'):
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 830; return DP_TZ; }
  break;
 default:
  if(!TJS_iswalpha(InputPointer[1])) { InputPointer += 1; yylex->val = -1200; return DP_TZ; }
 }
 break;
case TJS_W('n'):
case TJS_W('N'):
 switch(InputPointer[1])
 {
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -230; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('f'):
 case TJS_W('F'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -330; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('o'):
 case TJS_W('O'):
  switch(InputPointer[2])
  {
  case TJS_W('r'):
  case TJS_W('R'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   break;
  case TJS_W('v'):
  case TJS_W('V'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 10; return DP_MONTH; }
    break;
   case TJS_W('e'):
   case TJS_W('E'):
    switch(InputPointer[4])
    {
    case TJS_W('m'):
    case TJS_W('M'):
     switch(InputPointer[5])
     {
     case TJS_W('b'):
     case TJS_W('B'):
      switch(InputPointer[6])
      {
      case TJS_W('e'):
      case TJS_W('E'):
       switch(InputPointer[7])
       {
       case TJS_W('r'):
       case TJS_W('R'):
         if(!TJS_iswalpha(InputPointer[8])) { InputPointer += 8; yylex->val = 10; return DP_MONTH; }
        break;
       }
       break;
      }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 10; return DP_MONTH; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -330; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('t'):
 case TJS_W('T'):
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = -1100; return DP_TZ; }
  break;
 case TJS_W('z'):
 case TJS_W('Z'):
  switch(InputPointer[2])
  {
  case TJS_W('d'):
  case TJS_W('D'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1300; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1200; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 1200; return DP_TZ; }
   break;
  }
  break;
 default:
  if(!TJS_iswalpha(InputPointer[1])) { InputPointer += 1; yylex->val = 100; return DP_TZ; }
 }
 break;
case TJS_W('o'):
case TJS_W('O'):
 switch(InputPointer[1])
 {
 case TJS_W('c'):
 case TJS_W('C'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 9; return DP_MONTH; }
    break;
   case TJS_W('o'):
   case TJS_W('O'):
    switch(InputPointer[4])
    {
    case TJS_W('b'):
    case TJS_W('B'):
     switch(InputPointer[5])
     {
     case TJS_W('e'):
     case TJS_W('E'):
      switch(InputPointer[6])
      {
      case TJS_W('r'):
      case TJS_W('R'):
        if(!TJS_iswalpha(InputPointer[7])) { InputPointer += 7; yylex->val = 9; return DP_MONTH; }
       break;
      }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 9; return DP_MONTH; }
   }
   break;
  }
  break;
 }
 break;
case TJS_W('p'):
case TJS_W('P'):
 switch(InputPointer[1])
 {
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -700; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('m'):
 case TJS_W('M'):
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 0; return DP_PM; }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -800; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('s'):
case TJS_W('S'):
 switch(InputPointer[1])
 {
 case TJS_W('a'):
 case TJS_W('A'):
  switch(InputPointer[2])
  {
  case TJS_W('d'):
  case TJS_W('D'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 1030; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 930; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('t'):
  case TJS_W('T'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 6; return DP_WDAY; }
    break;
   case TJS_W('u'):
   case TJS_W('U'):
    switch(InputPointer[4])
    {
    case TJS_W('r'):
    case TJS_W('R'):
     switch(InputPointer[5])
     {
     case TJS_W('d'):
     case TJS_W('D'):
      switch(InputPointer[6])
      {
      case TJS_W('a'):
      case TJS_W('A'):
       switch(InputPointer[7])
       {
       case TJS_W('y'):
       case TJS_W('Y'):
         if(!TJS_iswalpha(InputPointer[8])) { InputPointer += 8; yylex->val = 6; return DP_WDAY; }
        break;
       }
       break;
      }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 6; return DP_WDAY; }
   }
   break;
  }
  break;
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('p'):
  case TJS_W('P'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 8; return DP_MONTH; }
    break;
   case TJS_W('t'):
   case TJS_W('T'):
    switch(InputPointer[4])
    {
    case TJS_W('.'):
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 8; return DP_MONTH; }
     break;
    case TJS_W('e'):
    case TJS_W('E'):
     switch(InputPointer[5])
     {
     case TJS_W('m'):
     case TJS_W('M'):
      switch(InputPointer[6])
      {
      case TJS_W('b'):
      case TJS_W('B'):
       switch(InputPointer[7])
       {
       case TJS_W('e'):
       case TJS_W('E'):
        switch(InputPointer[8])
        {
        case TJS_W('r'):
        case TJS_W('R'):
          if(!TJS_iswalpha(InputPointer[9])) { InputPointer += 9; yylex->val = 8; return DP_MONTH; }
         break;
        }
        break;
       }
       break;
      }
      break;
     }
     break;
    default:
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 8; return DP_MONTH; }
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 8; return DP_MONTH; }
   }
   break;
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 200; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('u'):
 case TJS_W('U'):
  switch(InputPointer[2])
  {
  case TJS_W('n'):
  case TJS_W('N'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 0; return DP_WDAY; }
    break;
   case TJS_W('d'):
   case TJS_W('D'):
    switch(InputPointer[4])
    {
    case TJS_W('a'):
    case TJS_W('A'):
     switch(InputPointer[5])
     {
     case TJS_W('y'):
     case TJS_W('Y'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 0; return DP_WDAY; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 0; return DP_WDAY; }
   }
   break;
  }
  break;
 case TJS_W('w'):
 case TJS_W('W'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 100; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('t'):
case TJS_W('T'):
 switch(InputPointer[1])
 {
 case TJS_W('h'):
 case TJS_W('H'):
  switch(InputPointer[2])
  {
  case TJS_W('u'):
  case TJS_W('U'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 4; return DP_WDAY; }
    break;
   case TJS_W('r'):
   case TJS_W('R'):
    switch(InputPointer[4])
    {
    case TJS_W('s'):
    case TJS_W('S'):
     switch(InputPointer[5])
     {
     case TJS_W('.'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 4; return DP_WDAY; }
      break;
     case TJS_W('d'):
     case TJS_W('D'):
      switch(InputPointer[6])
      {
      case TJS_W('a'):
      case TJS_W('A'):
       switch(InputPointer[7])
       {
       case TJS_W('y'):
       case TJS_W('Y'):
         if(!TJS_iswalpha(InputPointer[8])) { InputPointer += 8; yylex->val = 4; return DP_WDAY; }
        break;
       }
       break;
      }
      break;
     default:
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 4; return DP_WDAY; }
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 4; return DP_WDAY; }
   }
   break;
  }
  break;
 case TJS_W('u'):
 case TJS_W('U'):
  switch(InputPointer[2])
  {
  case TJS_W('e'):
  case TJS_W('E'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 2; return DP_WDAY; }
    break;
   case TJS_W('s'):
   case TJS_W('S'):
    switch(InputPointer[4])
    {
    case TJS_W('.'):
      if(!TJS_iswalpha(InputPointer[5])) { InputPointer += 5; yylex->val = 2; return DP_WDAY; }
     break;
    case TJS_W('d'):
    case TJS_W('D'):
     switch(InputPointer[5])
     {
     case TJS_W('a'):
     case TJS_W('A'):
      switch(InputPointer[6])
      {
      case TJS_W('y'):
      case TJS_W('Y'):
        if(!TJS_iswalpha(InputPointer[7])) { InputPointer += 7; yylex->val = 2; return DP_WDAY; }
       break;
      }
      break;
     }
     break;
    default:
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 2; return DP_WDAY; }
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 2; return DP_WDAY; }
   }
   break;
  }
  break;
 }
 break;
case TJS_W('u'):
case TJS_W('U'):
 switch(InputPointer[1])
 {
 case TJS_W('t'):
 case TJS_W('T'):
  switch(InputPointer[2])
  {
  case TJS_W('c'):
  case TJS_W('C'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 0; return DP_TZ; }
   break;
  default:
   if(!TJS_iswalpha(InputPointer[2])) { InputPointer += 2; yylex->val = 0; return DP_TZ; }
  }
  break;
 }
 break;
case TJS_W('w'):
case TJS_W('W'):
 switch(InputPointer[1])
 {
 case TJS_W('a'):
 case TJS_W('A'):
  switch(InputPointer[2])
  {
  case TJS_W('d'):
  case TJS_W('D'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 800; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('s'):
  case TJS_W('S'):
   switch(InputPointer[3])
   {
   case TJS_W('t'):
   case TJS_W('T'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 700; return DP_TZ; }
    break;
   }
   break;
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -100; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 900; return DP_TZ; }
   break;
  }
  break;
 case TJS_W('e'):
 case TJS_W('E'):
  switch(InputPointer[2])
  {
  case TJS_W('d'):
  case TJS_W('D'):
   switch(InputPointer[3])
   {
   case TJS_W('.'):
     if(!TJS_iswalpha(InputPointer[4])) { InputPointer += 4; yylex->val = 3; return DP_WDAY; }
    break;
   case TJS_W('n'):
   case TJS_W('N'):
    switch(InputPointer[4])
    {
    case TJS_W('e'):
    case TJS_W('E'):
     switch(InputPointer[5])
     {
     case TJS_W('s'):
     case TJS_W('S'):
      switch(InputPointer[6])
      {
      case TJS_W('d'):
      case TJS_W('D'):
       switch(InputPointer[7])
       {
       case TJS_W('a'):
       case TJS_W('A'):
        switch(InputPointer[8])
        {
        case TJS_W('y'):
        case TJS_W('Y'):
          if(!TJS_iswalpha(InputPointer[9])) { InputPointer += 9; yylex->val = 3; return DP_WDAY; }
         break;
        }
        break;
       }
       break;
      }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 3; return DP_WDAY; }
   }
   break;
  case TJS_W('t'):
  case TJS_W('T'):
   switch(InputPointer[3])
   {
   case TJS_W('d'):
   case TJS_W('D'):
    switch(InputPointer[4])
    {
    case TJS_W('s'):
    case TJS_W('S'):
     switch(InputPointer[5])
     {
     case TJS_W('t'):
     case TJS_W('T'):
       if(!TJS_iswalpha(InputPointer[6])) { InputPointer += 6; yylex->val = 100; return DP_TZ; }
      break;
     }
     break;
    }
    break;
   default:
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 0; return DP_TZ; }
   }
   break;
  }
  break;
 case TJS_W('s'):
 case TJS_W('S'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = 800; return DP_TZ; }
   break;
  }
  break;
 }
 break;
case TJS_W('y'):
case TJS_W('Y'):
 switch(InputPointer[1])
 {
 case TJS_W('d'):
 case TJS_W('D'):
  switch(InputPointer[2])
  {
  case TJS_W('t'):
  case TJS_W('T'):
    if(!TJS_iswalpha(InputPointer[3])) { InputPointer += 3; yylex->val = -800; return DP_TZ; }
   break;
  }
  break;
 default:
  if(!TJS_iswalpha(InputPointer[1])) { InputPointer += 1; yylex->val = 1200; return DP_TZ; }
 }
 break;
case TJS_W('z'):
case TJS_W('Z'):
  if(!TJS_iswalpha(InputPointer[1])) { InputPointer += 1; yylex->val = 0; return DP_TZ; }
 break;
}
