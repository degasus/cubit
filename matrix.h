#ifndef MATRIX_H
#define MATRIX_H

#include <iostream>
#include <cmath>
#include <assert.h>

template<typename T, int width, int height> class Matrix;
template<typename T, int dim> class LGSCH;
template<typename T, int dim> class LGSLU;


template<typename T, int dim> class LGSCH {
  public:
    Matrix<T,dim,dim> A;

    // --------------------
    // -- CH - Zerlegung --
    // --------------------
    inline LGSCH(Matrix<T,dim,dim> const &a) : A(a) {
      for(int j=0;j<dim;j++)
        for(int i=0; i<j; i++) {
          T h = A[i][j];
          A[i][j] = h/A[i][i];
          for(int k=i+1;k<=j;k++)
            A[k][j] -= h*A[i][k];
        }
    }

    // ----------------------------------
    // -- Rück- und Vorwärtseinsetzung --
    // ----------------------------------
    template<int width> inline Matrix<T,width,dim> solve(Matrix<T,width,dim> const &b) {
      Matrix<T,width,dim> x;

      for(int n=0; n<width; n++) {
        for(int j=0; j<dim; j++) {
          x[n][j] = b.data[n][j];
          for(int i=0; i<j; i++)
            x[n][j] -= A[i][j]*x[n][i];
        }
        for(int j=0;j<dim;j++)
          x[n][j] /= A[j][j];
        for(int j=dim-1;j>=0;j--)
          for(int i=j+1; i<dim; i++)
            x[n][j] -= A[j][i]*x[n][i];
      }
      return x;
    }

};

template<typename T, int dim> class LGSLU {
  public:
    Matrix<int,dim,1> P;
    Matrix<T,dim,dim> A;

    // --------------------
    // -- LU - Zerlegung --
    // --------------------
    LGSLU(Matrix<T,dim,dim> const &a) : A(a) {
		 
		 A = A.t();
      // P initialisieren
      for(int i=0; i<dim; i++)
        P[i][0] = i;

      for(int j=0; j<dim-1; j++){

        // Pivotsuche
        T a_max = 0;
        int a_max_pos = 0;
        for(int i=j; i<dim; i++) {
          T a_abs = std::abs(A[i][j]);
          if(a_max < a_abs) {
            a_max = a_abs;
            a_max_pos = i;
          }
        }
        // singulär
        assert(a_max != 0);
        // tausche Zeilen
        if(j != a_max_pos) {
          T tmp;
          for(int i=0; i<dim; i++) {
            tmp = A[j][i];
            A[j][i] = A[a_max_pos][i];
            A[a_max_pos][i] = tmp;
          }
          tmp = P[j][0];
          P[j][0] = P[a_max_pos][0];
          P[a_max_pos][0] = tmp;
        }
        //Teilen
        for(int i=j+1; i<dim; i++) {
          A[i][j] /= A[j][j];
          for(int k = j+1; k<dim; k++) {
            A[i][k] -= A[j][k] * A[i][j];
          }
        }
      }
    }

    // ----------------------------------
    // -- Rück- und Vorwärtseinsetzung --
    // ----------------------------------
    template<int width> inline Matrix<T,width,dim> solve(Matrix<T,width,dim> const &b) {
      Matrix<T,width,dim> x;

      for(int n=0; n<width; n++) {
        for(int i=0; i<dim; i++) {
          x[n][i] = b.data[n][P[i][0]];
          for(int j=0; j<i; j++)
            x[n][i] -= A[i][j]*x[n][j];
        }
        for(int i=dim-1; i>=0; i--) {
          for(int j=i+1; j<dim; j++)
            x[n][i] -= A[i][j]*x[n][j];
          x[n][i] /= A[i][i];
        }
      }
      return x;
    }
};

template<typename T, int width, int height>
class Matrix {
  public:
    T data[width][height];

    inline void set(T wert) {
      for(int x=0; x<width; x++)
        for(int y=0; y<height; y++)
          data[x][y] = wert;
    }

    inline Matrix(T wert) {
      set(wert);
    }

    inline Matrix() {}

    inline T* operator[](int pos) {
      return data[pos];
    }

    // ------------------------------------
    // -- Plus und Minus dem selben Typs --
    // ------------------------------------
    inline Matrix<T,width,height>& operator+=(Matrix<T,width,height> const &lvalue) {
      for(int x=0; x<width; x++)
        for(int y=0; y<height; y++)
          data[x][y] += lvalue.data[x][y];
      return *this;
    }
    inline Matrix<T,width,height>& operator-=(Matrix<T,width,height> const &lvalue) {
      for(int x=0; x<width; x++)
        for(int y=0; y<height; y++)
          data[x][y] -= lvalue.data[x][y];
      return *this;
    }
    inline Matrix<T,width,height> operator+(Matrix<T,width,height> const &lvalue) {
      return Matrix<T,width,height>(*this)+=lvalue;
    }
    inline Matrix<T,width,height> operator-(Matrix<T,width,height> const &lvalue) {
      return Matrix<T,width,height>(*this)-=lvalue;
    }

    // ----------------
    // -- Skalierung --
    // ----------------
    inline Matrix<T,width,height>& operator*=(T const &lvalue) {
      for(int x=0; x<width; x++)
        for(int y=0; y<height; y++)
          data[x][y] *= lvalue;
      return *this;
    }
    inline Matrix<T,width,height> operator*(T const &lvalue) {
      return Matrix<T,width,height>(*this)*=lvalue;
    }

    // ---------------------------
    // -- Matrix Multiplikation --
    // ---------------------------

    template<int lwidth> inline Matrix<T,lwidth,height> operator*(Matrix<T,lwidth,width> const &lvalue) {
      Matrix<T,lwidth,height> obj(T(0));
      for(int x=0; x<lwidth; x++)
        for(int y=0; y<height; y++)
          for(int i = 0; i < width; i++)
            obj.data[x][y] += data[i][y] * lvalue.data[x][i];

      return obj;
    }

    // --------------------
    // -- Transponierung --
    // --------------------
    // TODO: kopiert viel zu viel hin und her
    inline Matrix<T,height,width> t() {
      Matrix<T,height,width> obj;
      for(int x=0; x<height; x++)
        for(int y=0; y<width; y++)
          obj.data[x][y] = data[y][x];

      return obj;
    }

    // ---------
    // -- LGS --
    // ---------
    inline LGSLU<T,height> LU() {
      return LGSLU<T,height>(*this);
    }

    inline LGSCH<T,height> CH() {
      return LGSCH<T,height>(*this);
    }

    // -------------------------
    // -- Als String ausgeben --
    // -------------------------
    void to_str() {
      for(int y=0; y<height; y++) {
        std::cout << "[";
        for(int x=0; x<width; x++) {
          std::cout << " " << data[x][y];
        }
        std::cout << " ]" << std::endl;
      }
    }

    void read() {
      for(int y=0; y<height; y++)
        for(int x=0; x<width; x++)
          std::cin >> data[x][y];
    }

};


#endif
