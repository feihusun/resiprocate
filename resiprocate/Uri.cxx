#include <sipstack/Uri.hxx>
#include <util/ParseBuffer.hxx>
#include <sipstack/Symbols.hxx>

using namespace Vocal2;

Uri::Uri() 
   : ParserCategory(),
     mScheme(Symbols::DefaultSipScheme),
     mPort(0)
{}

Uri::Uri(const Uri& rhs)
   : ParserCategory(rhs),
     mScheme(rhs.mScheme),
     mHost(rhs.mHost),
     mUser(rhs.mUser),
     mPort(rhs.mPort),
     mPassword(rhs.mPassword)
{}

Uri&
Uri::operator=(const Uri& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mScheme = rhs.mScheme;
      mHost = rhs.mHost;
      mUser = rhs.mUser;
      mPort = rhs.mPort;
      mPassword = rhs.mPassword;
   }
   return *this;
}
   
bool 
Uri::operator==(const Uri& other) const
{
   if (isEqualNoCase(mScheme, other.mScheme) &&
       isEqualNoCase(mHost, other.mHost) &&
       mUser == other.mUser,
       mPassword == other.mPassword,
       mPort == other.mPort)
   {
      for (ParameterList::iterator it = mParameters.begin(); it != mParameters.end(); it++)
      {
         Parameter* otherParam = other.getParameterByEnum((*it)->getType());            
         switch ((*it)->getType())
         {
            case ParameterTypes::user:
            {
               if(!(otherParam &&
                    dynamic_cast<DataParameter*>(*it)->value() == 
                    dynamic_cast<DataParameter*>(otherParam)->value()))
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::ttl:
            {
               if(!(otherParam &&
                    dynamic_cast<IntegerParameter*>(*it)->value() == 
                    dynamic_cast<IntegerParameter*>(otherParam)->value()))
               {
                  return false;
               }
            }
            case ParameterTypes::method:
            {
               if(!(otherParam &&
                    dynamic_cast<DataParameter*>(*it)->value() == 
                    dynamic_cast<DataParameter*>(otherParam)->value()))
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::maddr:
            {               
               if(!(otherParam &&
                    dynamic_cast<DataParameter*>(*it)->value() == 
                    dynamic_cast<DataParameter*>(otherParam)->value()))
               {
                  return false;
               }
            }
            break;
            //the parameters that follow don't affect comparison if only present
            //in one of the URI's
            case ParameterTypes::transport:
            {
               if(otherParam &&
                  dynamic_cast<DataParameter*>(*it)->value() == 
                  dynamic_cast<DataParameter*>(otherParam)->value())
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::lr:
               break;
            default:
               break;
               //treat as unknown parameter?
         }
      }         
   }
   else
   {
      return false;
   }
   return true;
}


const Data&
Uri::getAor() const
{
   // did anything change?
   if (mOldScheme != mScheme ||
       mOldHost != mHost ||
       mOldUser != mUser ||
       mOldPort != mPort)
   {
      mOldScheme = mScheme;
      mOldHost = mHost;
      mOldUser = mUser;
      mOldPort = mPort;
       
      if (mUser.empty())
      {
         mAor = mHost;
      }
      else
      {
         mAor = mUser + Symbols::AT_SIGN + mHost;
      }

      if (mPort != 0)
      {
         mAor += Data(Symbols::COLON) + Data(mPort);
      }
   }
   return mAor;
}

void
Uri::parse(ParseBuffer& pb)
{
   const char* start = pb.position();
   pb.skipToChar(Symbols::COLON[0]);
   pb.data(mScheme, start);
   pb.skipChar();
   if (mScheme == Symbols::Sip || mScheme == Symbols::Sips)
   {
      start = pb.position();
      pb.skipToChar(Symbols::AT_SIGN[0]);
      if (!pb.eof())
      {
         pb.reset(start);
         start = pb.position();
         pb.skipToOneOf(":@");
         pb.data(mUser, start);
         if (*pb.position() == Symbols::COLON[0])
         {
            start = pb.skipChar();
            pb.skipToChar(Symbols::AT_SIGN[0]);
            pb.data(mPassword, start);
         }
         start = pb.skipChar();
      }
      else
      {
         pb.reset(start);
      }

      if (*start == '[')
      {
         pb.skipToChar(']');
      }
      else
      {
         pb.skipToOneOf(ParseBuffer::Whitespace, ":;>");
      }
      pb.data(mHost, start);
      pb.skipToOneOf(ParseBuffer::Whitespace, ":;>");
      if (!pb.eof() && *pb.position() == ':')
      {
         start = pb.skipChar();
         mPort = pb.integer();
         pb.skipToOneOf(ParseBuffer::Whitespace, ";>");
      }
      else
      {
         mPort = 0;
      }
   }
   else
   {
      // generic URL
      assert(0);
   }
}

ParserCategory*
Uri::clone() const
{
   return new Uri(*this);
}
 
std::ostream& 
Uri::encode(std::ostream& str) const
{
   str << mScheme << Symbols::COLON; 
   if (!mUser.empty())
   {
      str << mUser;
      if (!mPassword.empty())
      {
         str << Symbols::COLON << mPassword;
      }
      str << Symbols::AT_SIGN;
   }
   str << mHost;
   if (mPort != 0)
   {
      str << Symbols::COLON << mPort;
   }
   encodeParameters(str);
   return str;
}

void
Uri::parseEmbeddedHeaders()
{
   assert(0);
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
