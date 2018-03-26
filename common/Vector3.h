/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef VECTOR3_H
#define VECTOR3_H

#include <cstdio>
#include <iostream>
#include <cmath>
#include <cassert>
#include <algorithm>

namespace Math{

	class Vector3{
		public:
			double x, y, z;

			Vector3(){
				x=0.0; y=0.0; z=0.0;
			};
			Vector3(const double X, const double Y=0.0, const double Z=0.0){
				x=X;
				y=Y;
				z=Z;
			};
			Vector3(const Vector3& that){
	 //x=v.x;
	 //y=v.y;
	 //z=v.z;
				*this = that;
			};
			Vector3(const double v[3]){
				x=v[0];
				y=v[1];
				z=v[2];
			};
			
			Vector3(const Vector3 &p1, const Vector3 &p2){
				x=p2.x-p1.x;
				y=p2.y-p1.y;
				z=p2.z-p1.z;
			}

			double operator [] (int i) const{
				assert(i>=0 && i<3);
				if(i==0) return x;
				else if(i==1) return y;
				else if(i==2) return z;
			};
			double& operator [] (const int i){
				assert(i>=0 && i<3);
				if(i==0) return x;
				else if(i==1) return y;
				else if(i==2) return z;
			};
			double operator () (int i) const{
				assert(i>=1 && i<=3);
				if(i==1) return x;
				else if(i==2) return y;
				else if(i==3) return z;
			};
			double& operator () (const int i){
				assert(i>=1 && i<=3);
				if(i==1) return x;
				else if(i==2) return y;
				else if(i==3) return z;
			};
			
			void set(const double X, const double Y, const double Z){
				x=X;
				y=Y;
				z=Z;
			};
			void set(double v[3]){
				x=v[0];
				y=v[1];
				z=v[2];
			};
  
//			bool operator == (const Vector3& rhs)const
//			{
//				if(std::fabs(x-rhs.x)<eps && fabs(y-rhs.y)<eps && fabs(z-rhs.z)<eps)
//					return true;
//				return false;
//			}

//			bool operator != (const Vector3& rhs)const{
//				return !(*this==rhs);
//			}
	 
			Vector3& operator = (const Vector3& rhs){
				x=rhs.x;
				y=rhs.y;
				z=rhs.z;
				return(*this);
			};

			Vector3& operator = (double rhs){
				x=rhs;
				y=rhs;
				z=rhs;
				return *this;
			};


			Vector3 operator + (const Vector3& A) const{
				return(Vector3(x+A.x, y+A.y, z+A.z));
			};
			Vector3 operator - (const Vector3& A) const{
				return(Vector3(x-A.x, y-A.y, z-A.z));
			};
  
			Vector3 operator + (const double rhs) const{
				return Vector3(x+rhs, y+rhs, z+rhs);
			}
			Vector3 operator - (const double rhs) const{
				return Vector3(x-rhs, y-rhs, z-rhs);
			}


			double operator * (const Vector3& A) const{
				double DotProd = x*A.x+y*A.y+z*A.z;
				return(DotProd);
			};

    //   Vector3 operator / (const Vector3& A) const{
    // 	 Vector3 CrossProd(y*A.z-z*A.y, z*A.x-x*A.z, x*A.y-y*A.x);
    // 	 return(CrossProd);
    //   };

			Vector3 operator * (const double s) const{
				Vector3 Scaled(x*s, y*s, z*s);
				return(Scaled);
			};
			friend inline Vector3 operator *(double s, const Vector3& v){// scalar*Vector3tor
				return Vector3(v.x*s, v.y*s, v.z*s);
			}
  
			Vector3 operator / (const double s) const{
				Vector3 Scaled(x/s, y/s, z/s);
				return(Scaled);
			};
  
			void operator += (const Vector3 A){
				x+=A.x;
				y+=A.y;
				z+=A.z;
			};
			void operator -= (const Vector3 A){
				x-=A.x;
				y-=A.y;
				z-=A.z;
			};
			void operator *= (const double s){
				x*=s;
				y*=s;
				z*=s;
			};
			void operator /= (const double s){
				assert(s);
				x/=s;
				y/=s;
				z/=s;
			};
			Vector3 operator - (void) const{
				Vector3 Negated(-x, -y, -z);
				return(Negated);
			};
  
			double operator [] (const long i) const{
				return( (i==0)?x:((i==1)?y:z) );
			};
			double & operator [] (const long i){
				return( (i==0)?x:((i==1)?y:z) );
			};
  
			double norm2()const{
				double res = x*x + y*y + z*z;
				return res;
			}
			double norm()const{
				double n2 = norm2();
	 //assert(n2>0.0);
	 //if(n2<0.0)
	 //fprintf(stderr, "Warning, precision problems");
				return sqrt(std::max(n2,0.0));
			}
  
			void normalize(void){
				double L = norm();
	 //assert(L>0.0);
	 //if(L<=0.0)
	 //fprintf(stderr, "Warning, L<=0.0 (precision problems?)");
				x/=L;
				y/=L;
				z/=L;
			};
  
			Vector3 normal2d_l() const{
				return(Vector3(-y,x, 0));
			};
			Vector3 normal2d_r() const{
				return(Vector3(y,-x, 0));
			};
  
  //*VERY* BAD IDEA, called a few times in series invalidates earlier values
//     double* array3d()const{
// 		//static double arr[3] = {x,y,z};
// 		static double *arr = new double[3];
// 		arr[0] = x, arr[1] = y, arr[2] = z;
// 		return arr;
// 	 }
							
							
							
			void getArray3d(double arr[3])const{
				arr[0] = x, arr[1] = y, arr[2] = z;
			}
  
  //projection of *this onto direction given by arbitrary vector v
  //proper length, original (source) direction preserved
			Vector3 projectedOnVector(const Vector3& v)const{
	 //return(Vector3(*this*v/v.norm()*v.unit()));
				return (*this*v)*v/v.norm2();//faster than above
			};
  
  //projection of *this onto direction given by unit vector u
  //proper length, original (source) direction preserved
  //can be used if argument is known to be unit
  //or when projection length doesnt matter, only direction
			Vector3 projectedOnUnitVector(const Vector3& u)const{
				return (*this*u)*u;
			};
  
			Vector3 unit()const{
				double L=norm();
	 //assert(L>0.0);
	 //if(L<=0)
	 //fprintf(stderr,"Warning, L<=0.0 (precision problems?)");
				return(Vector3(x/L,y/L,z/L));
			};
  
  
			Vector3 vectorProduct(const Vector3& A) const{
				return(Vector3(y*A.z-z*A.y, z*A.x-x*A.z, x*A.y-y*A.x));
			};
			double scalarProduct(const Vector3& A) const{
				return(A.x*x+A.y*y+A.z*z);
			};
  
			friend std::ostream& operator << (std::ostream& os, const Vector3& v){
				os << v.x << " " << v.y << " " << v.z;
				return os;
			}
  

			//bool nan()const{
			//	return isnan(x) || isnan(y) || isnan(z);
			//}

			//static const Vector3* null_ptr;
			//static const Vector3& null_ref;

	};

}

#endif
