/*
 * Copyright (c) ICG. All rights reserved.
 *
 * Institute for Computer Graphics and Vision
 * Graz University of Technology / Austria
 *
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the above copyright notices for more information.
 *
 *
 * Project     : ImageUtilities
 * Module      : Core
 * Class       : LinearDeviceMemory
 * Language    : C++
 * Description : Inline implementation of a linear device memory class
 *
 * Author     : Manuel Werlberger
 * EMail      : werlberger@icg.tugraz.at
 *
 */

#ifndef IUSPARSE_SPARSEMATRIX_GPU_H
#define IUSPARSE_SPARSEMATRIX_GPU_H

#include <cusparse.h>

#include <cuda_runtime_api.h>
#include <iucore/lineardevicememory.h>
#include "sparsematrix_cpu.h"

namespace iu {

#define CUSPARSE_SAFE_CALL(x)   if ((x) != CUSPARSE_STATUS_SUCCESS) {fprintf( stderr, "CUSPARSE ERROR\n" ); return; }

template<typename PixelType>
  class SparseMatrixGpu
  {
  public:
    // Empty constructor
    SparseMatrixGpu(cusparseHandle_t* handle) :
        handle_(handle), n_row_(0), n_col_(0), n_elements_(0),
        value_(0), row_(0), col_(0), ext_data_pointer_(false)
    {
      createMatDescriptor();
    }

    // Destructor
    virtual ~SparseMatrixGpu()
    {
      if((!ext_data_pointer_) && (value_!=NULL))
      {
        delete value_;
        value_ = 0;
      }
      if((!ext_data_pointer_) && (row_!=NULL))
      {
        delete row_;
        row_ = 0;
      }
      if((!ext_data_pointer_) && (col_!=NULL))
      {
        delete col_;
        col_ = 0;
      }

      CUSPARSE_SAFE_CALL(cusparseDestroyMatDescr(A_));
    }

    // Constructor that takes host matrix as input
    SparseMatrixGpu(cusparseHandle_t* handle, SparseMatrixCpu<PixelType>* input) :
        handle_(handle), n_row_(0), n_col_(0), n_elements_(0),
        value_(0), row_(0), col_(0), ext_data_pointer_(false)
    {
      if (input == 0)
        return;

      // store size
      n_elements_ = input->n_elements();
      n_row_ = input->n_row();
      n_col_ = input->n_col();

      // allocate device memory
      value_ = new LinearDeviceMemory<PixelType>(input->value()->length());
      row_ = new LinearDeviceMemory<int>(input->row()->length());
      col_ = new LinearDeviceMemory<int>(input->col()->length());

      // copy form host to device
      cudaMemcpy(value_->data(), input->value()->data(),
                 input->value()->length()*sizeof(PixelType), cudaMemcpyHostToDevice);
      cudaMemcpy(row_->data(), input->row()->data(),
                 input->row()->length()*sizeof(int), cudaMemcpyHostToDevice);
      cudaMemcpy(col_->data(), input->col()->data(),
                 input->col()->length()*sizeof(int), cudaMemcpyHostToDevice);

      createMatDescriptor();
    }

    // Constructor based on preallocated memory
    SparseMatrixGpu(cusparseHandle_t* handle, LinearDeviceMemory<PixelType>* value,
                    LinearDeviceMemory<int>* row,
                    LinearDeviceMemory<int>* col, int n_row, int n_col,
                    bool ext_data_pointer = false) :
       handle_(handle), value_(0), row_(0), col_(0), n_row_(n_row),
       n_col_(n_col), n_elements_(0), ext_data_pointer_(ext_data_pointer)
    {
      if(ext_data_pointer_)
      {
        // This uses the external memory as internal one.
        value_ = value;
        row_ = row;
        col_ = col;
      }
      else
      {
        // allocates internal memory
        if ((value == 0) || (row == 0) || (col == 0))
          return;

        value_ = new LinearDeviceMemory<PixelType>(*value);
        row_ = new LinearDeviceMemory<int>(*row);
        col_ = new LinearDeviceMemory<int>(*col);
      }

      n_elements_ = value->length();

      createMatDescriptor();
    }


    // Copy constructor
    SparseMatrixGpu(cusparseHandle_t* handle, SparseMatrixGpu<PixelType>* input,
                    bool transpose = false) :
        handle_(handle), n_row_(0), n_col_(0), n_elements_(0),
        value_(0), row_(0), col_(0), ext_data_pointer_(false)
    {
      if (input == 0)
        return;

      // store size
      n_elements_ = input->n_elements();
      n_row_ = input->n_row();
      n_col_ = input->n_col();

      if (transpose)
      {
        if (input->isCSC())
        {
          // allocate device memory
          value_ = new LinearDeviceMemory<PixelType>(n_elements_);
          row_ = new LinearDeviceMemory<int>(n_row_+1);
          col_ = new LinearDeviceMemory<int>(n_elements_);

          // Converts matrix in CSC format into matrix in CSR format
          CUSPARSE_SAFE_CALL(cusparseScsr2csc(*handle_, n_col_, n_row_, input->value()->data(),
                                              input->col()->data(), input->row()->data(),
                                              value_->data(), col_->data(), row_->data(), 1,
                                              CUSPARSE_INDEX_BASE_ZERO));
        }
        else if (input->isCSR())
        {
          printf("WARNING: DOES NOT WORK!!!\n");
          // allocate device memory
//          value_ = new LinearDeviceMemory<PixelType>(n_elements_);
//          row_ = new LinearDeviceMemory<int>(n_elements_);
//          col_ = new LinearDeviceMemory<int>(n_col_+1);

//          // Converts matrix in CSR format into matrix in CSC format
//          CUSPARSE_SAFE_CALL(cusparseScsr2csc(*handle_, n_col_, n_row_, input->value()->data(),
//                                              input->row()->data(), input->col()->data(),
//                                              value_->data(), row_->data(), col_->data(), 1,
//                                              CUSPARSE_INDEX_BASE_ZERO));
        }
      }
      else
      {
        // allocate device memory
        value_ = new LinearDeviceMemory<PixelType>(input->value()->length());
        row_ = new LinearDeviceMemory<int>(input->row()->length());
        col_ = new LinearDeviceMemory<int>(input->col()->length());

        // copy form input to this
        cudaMemcpy(value_->data(), input->value()->data(),
                   input->value()->length()*sizeof(PixelType), cudaMemcpyDeviceToDevice);
        cudaMemcpy(row_->data(), input->row()->data(),
                   input->row()->length()*sizeof(int), cudaMemcpyDeviceToDevice);
        cudaMemcpy(col_->data(), input->col()->data(),
                   input->col()->length()*sizeof(int), cudaMemcpyDeviceToDevice);
      }

      createMatDescriptor();
    }


    const LinearDeviceMemory<PixelType>* value() const
    {
      return reinterpret_cast<const LinearDeviceMemory<PixelType>*>(value_);
    }

    const LinearDeviceMemory<int>* row() const
    {
      return reinterpret_cast<const LinearDeviceMemory<int>*>(row_);
    }

    const LinearDeviceMemory<int>* col() const
    {
      return reinterpret_cast<const LinearDeviceMemory<int>*>(col_);
    }

    int n_elements() { return n_elements_; }
    int n_row() { return n_row_; }
    int n_col() { return n_col_; }
    cusparseMatDescr_t mat_descriptor() { return A_; }

    cusparseHandle_t* handle() { return handle_; }

    int isCSC()
    {
      if (col_ == 0)
        return false;
      return (n_col_ != col_->length());
    }

    int isCSR()
    {
      if (row_ == 0)
        return false;
      return (n_row_ != row_->length());
    }

  protected:

  private:
    void createMatDescriptor()
    {
      A_ = 0;
      CUSPARSE_SAFE_CALL(cusparseCreateMatDescr(&A_));
      CUSPARSE_SAFE_CALL(cusparseSetMatType(A_,CUSPARSE_MATRIX_TYPE_GENERAL));
      CUSPARSE_SAFE_CALL(cusparseSetMatIndexBase(A_,CUSPARSE_INDEX_BASE_ZERO));
    }

    cusparseHandle_t* handle_;

    int n_row_;      /**< Number of rows in the sparse matrix */
    int n_col_;      /**< Number of columns in the sparse matrix */
    int n_elements_; /**< Number of non-zero elements in the sparse matrix */

    LinearDeviceMemory<PixelType>* value_;
    LinearDeviceMemory<int>* row_;
    LinearDeviceMemory<int>* col_;

    cusparseMatDescr_t A_;

    bool ext_data_pointer_; /**< Flag if data pointer is handled outside the image class. */

  };


} // namespace iuprivate

#endif // IUSPARSE_SPARSEMATRIX_GPU_H
