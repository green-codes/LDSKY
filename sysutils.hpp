/*
 * System-level programs
 */

#ifndef SYSUTILS_H
#define SYSUTILS_H

#include "base.h"
#include "system.hpp"
#include "devices.hpp"

namespace SysUtils
{

/* ===== Input Window ===== */

class InputWindow
{
private:
  // device pointers
  Devices::LC_Display *lcd_;
  Devices::Keypad_I *keypad_;

  // private members
  char *inputbuf_;    // input buffer
  int counter_ = 0;   // input character counter
  int lc_addr_;       // input row index
  int offset_;        // input row offset
  int len_;           // input window length
  bool allow_cursor_; // whether to allow cursor keys (for changing row)

public:
  // public members
  enum iw_status // status flag
  {
    IW_INPUT,    // normal input
    IW_COMPLETE, // user exit via enter key
    IW_EXIT,     // user exit via exit key
    IW_EXIT_UP,  // user exit via up cursor
    IW_EXIT_DOWN // user exit via down cursor
  };
  iw_status status_ = IW_INPUT;
  enum iw_mode // input mode
  {
    IW_UL,   // unsigned long
    IW_LONG, // long
    IW_FLOAT // float
  };
  iw_mode mode_;
  void *p_res_; // return value pointer

  InputWindow(void *p_res, int lc_addr, int offset, int len, iw_mode mode,
              bool allow_cursor)
      : p_res_{p_res}, lc_addr_{lc_addr}, offset_{offset}, len_{len},
        mode_{mode}, allow_cursor_{allow_cursor}
  {
    lcd_ = Devices::lcd;
    keypad_ = Devices::keypad;
    inputbuf_ = calloc(len * 2, sizeof(char)); // NOTE: remember to free!
    memset(inputbuf_, '_', len_);

    lcd_->printStr(lc_addr_, inputbuf_, offset_, len_, NULL, NULL);
  }
  ~InputWindow()
  {
    free(inputbuf_); // free input buffer
  }

  // process input according to iw_status
  // Note: the input window is only responsible for writing data and flags;
  //       whoever instantiates the iw is responsible for destroying it
  iw_status process_input(char k)
  {
    if (status_ == IW_INPUT) // only process input if in IW_INPUT state
    {
      if (k == M_ENTR_KEY)
      {
        if (mode_ == IW_UL)
          *((unsigned long *)p_res_) = (unsigned long)atol(inputbuf_);
        else if (mode_ == IW_LONG)
          *((long *)p_res_) = atol(inputbuf_);
        else if (mode_ == IW_FLOAT)
          *((double *)p_res_) = atof(inputbuf_);
        status_ = IW_COMPLETE;
        return IW_COMPLETE;
      }
      else if (k == M_EXIT_KEY)
      {
        if (counter_ == 0) // input buffer empty
        {
          status_ = IW_EXIT;
          return IW_EXIT;
        }
        else // input buffer not empty, clear input area
        {
          memset(inputbuf_, '_', len_);
          counter_ = 0;
        }
      }
      else if (allow_cursor_ && k == M_UP_KEY)
      {
        status_ = IW_EXIT_UP;
        return IW_EXIT_UP;
      }
      else if (allow_cursor_ && k == M_DOWN_KEY)
      {
        status_ = IW_EXIT_DOWN;
        return IW_EXIT_DOWN;
      }
      else if (isdigit(k) || (k == '.' && mode_ == IW_FLOAT) ||
               (k == '-' && mode_ != IW_UL))
      {
        // if reached end of buffer, reset input area
        if (counter_ == len_)
        {
          memset(inputbuf_, '_', len_);
          counter_ = 0;
        }
        inputbuf_[counter_] = k;
        counter_ += 1;
      }
      else // unrecognized key input, just return the current status
        return status_;
      // middle of input session
      // print to screen directly, as the current row should be frozen
      lcd_->printStr(lc_addr_, inputbuf_, offset_, len_, NULL, NULL);
      return IW_INPUT;
    }
    else // not in input state, do nothing
      return status_;
  }
};

/* ===== system mananger ===== */

class SysManager
{

public:
  InputWindow *iw_;       // input window pointer
  bool iw_open_ = false;  // input window open flag
  long pgm_exec_time = 0; // execution time of last program cycle
  enum keyrel_req_t       // key release request tracker
  {
    KYRL_NULL,    // no key release requests (pgm respondible for clearing!)
    KYRL_REQ_PGM, // key release request (program)
    KYRL_REQ_VN,  // key release request (verb/noun)
    KYRL_REQ_PVN, // key release request (both)
    KYRL_ACC,     // key release request accepted
    KYRL_REJ      // key release request rejected
  } keyrel_req = KYRL_NULL;

  // ===== input window and UI managing =====

  // open an input window
  // creates an InputWindow instance and pass all parameters
  // NOTE: wrapper useful for keeping track of input window status
  // NOTE: does NOT freeze the display; the caller is responsible
  int input_window_open(void *p_res, int lc_addr, int offset, int len,
                        InputWindow::iw_mode mode, bool allow_cursor)
  {
    if (!iw_open_)
    {
      iw_ = new InputWindow(p_res, lc_addr, offset, len, mode, allow_cursor);
      iw_open_ = true;
      return 0;
    }
    else // request rejected; caller should raise PGM ERR
      return -1;
  }
  // close the input window
  // deletes the InputWindow instance
  // NOTE: does NOT unfreeze the display; caller is responsible
  int input_window_close()
  {
    if (iw_open_)
    {
      delete iw_; // may unfreeze the display row
      iw_open_ = false;
      return 0;
    }
    else // no input window is open
      return -1;
  }
  // decide where to send the registered key press
  void process_key_event()
  {
    char k = keypad_->getKeyEvent(); // NOTE: allows next key input
    if (k)
    {
      if (iw_open_) // if input window open, pass key event
        iw_->process_input(k);
      else // handle monitor UI
      {
        if (k == M_ENTR_KEY) // enter verb and noun
          vn_input_stage_ = VN_VERB;
        else if (k == M_EXIT_KEY) // clear error states
        {
          // stop verb if verb error
          if (verb_status_ == V_PGM_ERR || verb_status_ == V_OPR_ERR)
            stop_verb();
          // kill program if program returns error
          if (p_dict_[pvn_state_[0]].status == P_PGM_ERR ||
              p_dict_[pvn_state_[0]].status == P_OPR_ERR)
            kill_program();
          // reject key release
          key_release_rej();
          // clear error lights
          status_led_->clearError();
        }
        else if (k == M_UP_KEY) // key release
          key_release_acc();
      }
    }
  }

  // ===== Verb and noun managing =====
  enum verb_status_t // verb status enum
  {
    V_RUN = 1,
    V_COMPLETE = 0,
    V_PGM_ERR = -1,
    V_OPR_ERR = -2
  } verb_status_ = V_COMPLETE;
  struct VDict_t // verb registry entry
  {
    bool valid = false;              // verb valid
    int (*verb_ptr)(int *, void **); // pointer to the verb function
    void *data_ptr = NULL;           // data pointer
    int stage = 0;                   // verb stage
    bool has_noun = false;           // whether the verb requires a noun
  };

  // add a verb to the verb registry
  int register_verb(int id, int (*verb_ptr)(int *, void **), bool has_noun)
  {
    if (!v_dict_[id].valid) // verb not found in registry
    {
      v_dict_[id].valid = true;
      v_dict_[id].verb_ptr = verb_ptr;
      v_dict_[id].has_noun = has_noun;
      return 0;
    }
    else // verb id already occupied
      return -1;
  }

  // if in verb/noun input mode, handle the input logic
  void process_vn_input()
  {
    if (vn_input_stage_ != VN_NULL) // verb/noun input active
    {
      lcd_->setUpdate(0, false);      // freeze p/v/n display
      if (vn_input_stage_ == VN_VERB) // verb input
      {
        if (!iw_open_) // open new window if neccesary
          input_window_open(&vn_buffer_, 0, 3, 2, InputWindow::IW_UL, false);
        if (iw_->status_ == InputWindow::IW_COMPLETE)
        {
          v_buf_ = (int)vn_buffer_; // read input buffer
          input_window_close();
          if (!v_dict_[v_buf_].valid) // verb not found
          {
            vn_input_stage_ = VN_NULL;
            status_led_->setStatus(LED_OPER_P, true); // operator error!
          }
          else if (v_dict_[v_buf_].has_noun) // verb found, has noun
          {
            vn_input_stage_ = VN_NOUN; // set noun input
          }
          else // verb found, no noun
          {
            vn_input_stage_ = VN_NULL; // set exit
            set_vn(v_buf_, 0);
          }
        }
        else if (iw_->status_ == InputWindow::IW_EXIT)
        {
          input_window_close();
          vn_input_stage_ = VN_NULL; // user exited verb input window
        }
        // else do nothing and wait for input window to finish
      }
      else if (vn_input_stage_ == VN_NOUN) // noun input
      {
        if (!iw_open_) // open new window if neccesary
          input_window_open(&vn_buffer_, 0, 6, 2, InputWindow::IW_UL, false);
        if (iw_->status_ == InputWindow::IW_COMPLETE)
        {
          input_window_close();
          vn_input_stage_ = VN_NULL;
          // NOTE: the verb is responsible for checking the noun
          set_vn(v_buf_, (int)vn_buffer_);
        }
        else if (iw_->status_ == InputWindow::IW_EXIT)
        {
          input_window_close();
          vn_input_stage_ = VN_VERB; // return to VERB input
        }
      }
    }
    else
      lcd_->setUpdate(0, true); // unfreeze p/v/n display
  }

  // mark a verb-noun pair for execution
  int set_vn(int v, int n)
  {
    // verb valid
    if (v_dict_[v].valid)
    {
      // reinit
      stop_verb();
      v_dict_[v].stage = 0;
      lcd_->clearDataRows(); // clear display on verb switch
      // set verb/noun
      pvn_state_[1] = v;
      pvn_state_[2] = n;
      pvn_state_pgm_[1] = v; // make sure we clear key release
      pvn_state_pgm_[2] = n;
      verb_status_ = V_RUN;      // set flag to allow verb running
      status_led_->clearError(); // clear error lights on v/n set
      return 0;
    }
    else // must be a pgm err; the usr can't enter invalid verbs
    {
      status_led_->setStatus(LED_PGER_P, true);
      return -1;
    }
  }
  int get_verb()
  {
    return pvn_state_[1];
  }
  int get_noun()
  {
    return pvn_state_[2];
  }
  // stop a verb; easy since verbs don't store anything
  // NOTE: may interfere with programs!
  void stop_verb()
  {
    free(v_dict_[pvn_state_[1]].data_ptr); // free verb memory
    v_dict_[pvn_state_[1]].stage = 0;
    verb_status_ = V_COMPLETE;
    pvn_state_[1] = 0; // reset v/n
    pvn_state_[2] = 0;
    // lcd_->clearDataRows(); // clear on verb switch only
  }

  // execute the current verb in pvn_state_
  // Note: the verb is respnsible for updating data displays
  void execute_verb()
  {
    int v = pvn_state_[1];
    if (v) // if current verb not null (zero)
    {
      if (!v_dict_[v].valid)
      {
        // a program must have set the invalid verb
        // i.e. program -> pvn_state_pgm_ -> key release
        status_led_->setStatus(LED_PGER_P, true);
        verb_status_ = V_PGM_ERR;
      }
      else // execute the verb
      {
        if (pvn_state_[2] && !v_dict_[v].has_noun)
          pvn_state_[2] = 0; // clear noun if current verb doesn't need one
        if (verb_status_ == V_RUN)
          verb_status_ = v_dict_[v].verb_ptr(&(v_dict_[v].stage),
                                             &(v_dict_[v].data_ptr));
        else if (verb_status_ == V_COMPLETE) // verb marked as complete
          stop_verb();
        else if (verb_status_ == V_PGM_ERR) // program error
          status_led_->setStatus(LED_PGER_P, true);
        else if (verb_status_ == V_OPR_ERR) // operator error
          status_led_->setStatus(LED_OPER_P, true);
      }
    }
    else
    {
      pvn_state_[2] = 0; // reset noun since we're on the null verb
      verb_status_ = V_COMPLETE;
    }
  }

  // ===== program managing =====

  enum pgm_status_t // pgm status enum (each program keeps one of these)
  {
    P_PAUSE = 2,    // program paused
    P_RUN = 1,      // program running
    P_COMPLETE = 0, // program complete, clean up
    P_PGM_ERR = -1,
    P_OPR_ERR = -2
  };
  struct PDict_t // program registry entry (pgm should have access to this)
  {
    bool valid = false;             // program id valid
    int (*pgm_ptr)(int *, void **); // pointer to the program main func
    int stage = 0;                  // program stage
    void *data_ptr = NULL;          // pointer to persistent data (free!)
    pgm_status_t status = P_RUN;    // program status flag
  };

  // add a program to the program registry
  int register_program(int id, int (*p_ptr)(int *, void **))
  {
    if (!p_dict_[id].valid) // verb not found in registry
    {
      p_dict_[id].valid = true;
      p_dict_[id].pgm_ptr = p_ptr;
      return 0;
    }
    else // program id already occupied
      return -1;
  }

  // set program
  int set_program(int id, bool kill_old)
  {
    // program valid
    if (p_dict_[id].valid)
    {
      // ignore switching to same program
      if (pvn_state_[0] != id)
      {
        if (kill_old)
          kill_program();
        else
          pause_program();

        // TODO: potentially more logic

        // switch program
        pvn_state_[0] = id;
        pvn_state_pgm_[0] = id; // make sure we clear key release
        if (p_dict_[id].status != P_PAUSE)
          p_dict_[id].stage = 0; // i.e. if paused, just resume
        p_dict_[id].status = P_RUN;
      }
      return 0;
    }
    else
      return -1;
  }
  int set_program(int id)
  {
    return set_program(id, true);
  }
  int get_program()
  {
    return pvn_state_[0];
  }
  // kill the current program
  int kill_program()
  {
    if (!p_dict_[pvn_state_[0]].valid) // pgm invalid
      return -1;
    free(p_dict_[pvn_state_[0]].data_ptr);      // free program storage
    p_dict_[pvn_state_[0]].status = P_COMPLETE; // clear error flag
    pvn_state_[0] = 0;                          // stop program
    return 0;
  }
  // pause and resume program
  int pause_program(int id)
  {
    if (!p_dict_[id].valid) // pgm invalid
      return -1;
    if (!(p_dict_[id].status == P_RUN)) // can't pause if not running
      return -1;
    p_dict_[id].status = P_PAUSE;
    return 0;
  }
  int pause_program()
  {
    return pause_program(pvn_state_[0]);
  }
  int resume_program(int id)
  {
    if (!p_dict_[id].valid) // pgm invalid
      return -1;
    if (!(p_dict_[id].status == P_PAUSE)) // can't resume if not paused
      return -1;
    p_dict_[id].status = P_RUN;
    return 0;
  }
  int resume_program()
  {
    return resume_program(pvn_state_[0]);
  }

  // step through the current program
  void step_program()
  {
    int pgm = pvn_state_[0];
    if (pgm) // program not null
    {
      if (!p_dict_[pgm].valid) // program DNE
        status_led_->setStatus(LED_PGER_P, true);
      else
      {
        PDict_t *curr_pgm = p_dict_ + pgm;
        if (curr_pgm->status == P_RUN) // execute program
        {
          status_led_->setActivityLED(true); // only programs get ACT lgt
          pgm_exec_time = millis();
          curr_pgm->status =
              curr_pgm->pgm_ptr(&(curr_pgm->stage), &(curr_pgm->data_ptr));
          pgm_exec_time = millis() - pgm_exec_time;
          status_led_->setActivityLED(false);
        }
        else
          pgm_exec_time = 0;
        if (curr_pgm->status == P_COMPLETE) // program complete
          kill_program();
        else if (curr_pgm->status == P_PGM_ERR) // program error
          status_led_->setStatus(LED_PGER_P, true);
        else if (curr_pgm->status == P_OPR_ERR) // operator error
          status_led_->setStatus(LED_OPER_P, true);
      }
    }
    // else (null program) do nothing
  }

  // update key release light according to request status
  void update_key_rel()
  {
    // if requested, mark key release
    if (keyrel_req == KYRL_REQ_VN ||
        keyrel_req == KYRL_REQ_PGM ||
        keyrel_req == KYRL_REQ_PVN)
      status_led_->setStatus(LED_KYRL_P, true);
    else // clear key release
      status_led_->setStatus(LED_KYRL_P, false);
  }
  void request_pvn(int pgm, int v, int n, bool force = false)
  {
    pvn_state_pgm_[0] = pgm;
    pvn_state_pgm_[1] = v;
    pvn_state_pgm_[2] = n;
    keyrel_req = KYRL_REQ_PVN;
    if (force)
      key_release_acc();
  }
  // request key release (v/n)
  void request_vn(int v, int n, bool force = false)
  {
    pvn_state_pgm_[1] = v;
    pvn_state_pgm_[2] = n;
    keyrel_req = KYRL_REQ_VN;
    if (force)
      key_release_acc();
  }
  // request key release (program)
  void request_pgm(int pgm, bool force = false)
  {
    pvn_state_pgm_[0] = pgm;
    keyrel_req = KYRL_REQ_PGM;
    if (force)
      key_release_acc();
  }
  // accept key release request, switch
  void key_release_acc()
  {
    if (keyrel_req == KYRL_REQ_VN ||
        keyrel_req == KYRL_REQ_PGM ||
        keyrel_req == KYRL_REQ_PVN) // ignore if no request
    {
      // set program: kill the current program
      // NOTE: a program is ready to be killed if it requests program change
      if (keyrel_req == KYRL_REQ_PGM ||
          keyrel_req == KYRL_REQ_PVN)
        set_program(pvn_state_pgm_[0], true); // kill on key release
      if (keyrel_req == KYRL_REQ_VN ||
          keyrel_req == KYRL_REQ_PVN)
        set_vn(pvn_state_pgm_[1], pvn_state_pgm_[2]); // set verb and noun
      keyrel_req = KYRL_ACC;
    }
  }
  // reject key releast request
  void key_release_rej()
  {
    if (keyrel_req == KYRL_REQ_VN ||
        keyrel_req == KYRL_REQ_PGM ||
        keyrel_req == KYRL_REQ_PVN)
      keyrel_req = KYRL_REJ;
  }

  // do system manager cycle;
  // should be called from the main loop
  void update() // the system manager reports to no one...
  {
    process_key_event();
    process_vn_input();
    lcd_->setPVN(0, pvn_state_); // set program, verb and noun display

    execute_verb(); // verb before program s.t. verb37 can work immediately
    step_program();
    update_key_rel(); // update key release light
  }

private:
  // device managing
  Devices::LC_Display *lcd_ = Devices::lcd;                  // display
  Devices::Keypad_I *keypad_ = Devices::keypad;              // keypad
  Devices::StatusDisplay *status_led_ = Devices::status_led; // status lgts

  // program/verb/noun selection managing
  int pvn_state_[3] = {0, 0, 0};     // Program, verb and noun states
  int pvn_state_pgm_[3] = {0, 0, 0}; // program-requested p/v/n states

  // verb registry
  VDict_t v_dict_[100]; // one entry for each verb

  // program registry
  // NOTE: since SysManager has the persistent storage pointer,
  //       we can safely kill the program whenever we want
  PDict_t p_dict_[100]; // one entry for each program

  // vern/noun input managing
  enum vn_in_stage // verb/noun input stage
  {
    VN_NULL, // not inputting verb/noun
    VN_VERB, // verb input
    VN_NOUN  // noun input
  } vn_input_stage_ = VN_NULL;
  unsigned long vn_buffer_;
  int v_buf_; // buffer for verb input
};
SysManager *sys;

} // namespace SysUtils

#endif